﻿#include "StateDetailView.h"
#include <Rendering/Camera/FTCamera2D.h>
#include <Rendering/Text/FTLabel.h>
#include <Util/FTStringUtils.h>
#include <Rendering/FTWindowSizeNode.h>
#include <telemetry.h>
#include <messaging.h>
#include <calibration/ms5611_calibration.h>
#include <calibration/mpu9250_calibration.h>
#include <calibration/adis16405_calibration.h>
#include <Event/Input/FTInputManager.h>
#include <state/kalman.h>
#include <state/math_util.h>


static StateDetailView* s_instance = nullptr;
static const int num_labels = 46;
float values[num_labels];

int mpu9250_update_count = 0;
int state_estimate_update_count = 0;
int ms5611_update_count = 0;

static bool getPacket(const telemetry_t* packet, message_metadata_t metadata) {
    if (s_instance == nullptr)
        return false;
    if (packet->header.id == ts_mpu9250_data) {
        mpu9250_data_t* data = (mpu9250_data_t*)packet->payload;

        mpu9250_calibrated_data_t calibrated;
        mpu9250_calibrate_data(data, &calibrated);

        values[3] = calibrated.accel[0];
        values[4] = calibrated.accel[1];
        values[5] = calibrated.accel[2];

        values[6] = calibrated.gyro[0];
        values[7] = calibrated.gyro[1];
        values[8] = calibrated.gyro[2];

        values[9] = calibrated.magno[0];
        values[10] = calibrated.magno[1];
        values[11] = calibrated.magno[2];

        values[12] = mpu9250_get_heading(&calibrated);

        values[43] = Eigen::Map<const Matrix<float, 3, 1>>(calibrated.magno).norm();
        values[45] = Eigen::Map<const Matrix<float, 3, 1>>(calibrated.accel).norm();

        mpu9250_update_count++;
    }
    else if (packet->header.id == ts_ms5611_data) {
        auto data = telemetry_get_payload<ms5611data_t>(packet);

        values[0] = (float)data->pressure;
        values[1] = ms5611_get_altitude(data);
        values[2] = (float)data->temperature;
        ms5611_update_count++;
    } else if (packet->header.id == ts_state_estimate_data) {
        state_estimate_update_count++;
        auto state = telemetry_get_payload<state_estimate_t>(packet);

        Eigen::Vector3f euler = quat_to_euler(Eigen::Quaternionf(state->orientation_q[3], state->orientation_q[0], state->orientation_q[1], state->orientation_q[2]));
        values[16] = euler.x();
        values[17] = euler.y();
        values[18] = euler.z();

        values[19] = state->angular_velocity[0];
        values[20] = state->angular_velocity[1];
        values[21] = state->angular_velocity[2];

        values[22] = state->position[0];
        values[23] = state->position[1];
        values[24] = state->position[2];

        values[25] = state->velocity[0];
        values[26] = state->velocity[1];
        values[27] = state->velocity[2];

        values[28] = state->acceleration[0];
        values[29] = state->acceleration[1];
        values[30] = state->acceleration[2];
    } else if (packet->header.id == ts_adis16405_data) {
        auto data = telemetry_get_payload<adis16405_data_t>(packet);
        adis16405_calibrated_data_t calibrated_data;
        adis16405_calibrate_data(data, &calibrated_data);

        values[31] = calibrated_data.supply;
        values[32] = calibrated_data.gyro[0];
        values[33] = calibrated_data.gyro[1];
        values[34] = calibrated_data.gyro[2];
        values[35] = calibrated_data.accel[0];
        values[36] = calibrated_data.accel[1];
        values[37] = calibrated_data.accel[2];
        values[38] = calibrated_data.magno[0];
        values[39] = calibrated_data.magno[1];
        values[40] = calibrated_data.magno[2];

        values[41] = std::acos(Eigen::Map<const Matrix<float, 3, 1>>(calibrated_data.accel).normalized().transpose() *
                                Eigen::Map<const Matrix<float, 3, 1>>(calibrated_data.magno).normalized());

        values[42] = Eigen::Map<const Matrix<float, 3, 1>>(calibrated_data.magno).norm();
        values[44] = Eigen::Map<const Matrix<float, 3, 1>>(calibrated_data.accel).norm();
    }
    return true;
}

MESSAGING_CONSUMER(messaging_consumer, ts_all, ts_all_mask, 0, 0, getPacket, 1024);

StateDetailView::StateDetailView() {
    FTAssert(s_instance == nullptr, "Only one StateDetailView instance can exist at once");
    
    auto camera = std::make_shared<FTCamera2D>();
    camera->setCullFaceEnabled(false);
    setCamera(std::move(camera));

    static const wchar_t* label_names[num_labels] = { 
        L"MS5611 Pressure", L"MS5611 Altitude", L"MS5611 Temperature",
        L"MPU9250 Accel X", L"MPU9250 Accel Y", L"MPU9250 Accel Z", 
        L"MPU9250 Gyro X", L"MPU9250 Gyro Y", L"MPU9250 Gyro Z", 
        L"MPU9250 Magno X", L"MPU9250 Magno Y", L"MPU9250 Magno Z",
        L"MPU9250 Heading", L"MPU9250 Update Rate", L"SE Update Rate", L"MS5611 Update Rate",
        L"SE Rotation X", L"SE Rotation Y", L"SE Rotation Z",
        L"SE Angular Velocity X", L"SE Angular Velocity Y", L"SE Angular Velocity Z",
        L"SE Position X", L"SE Position Y", L"SE Position Z",
        L"SE Velocity X", L"SE Velocity Y", L"SE Velocity Z",
        L"SE Acceleration X", L"SE Acceleration Y", L"SE Acceleration Z",
        L"ADIS Supply",
        L"ADIS Gyro X", L"ADIS Gyro Y", L"ADIS Gyro Z",
        L"ADIS Accel X", L"ADIS Accel Y", L"ADIS Accel Z",
        L"ADIS Magno X", L"ADIS Magno Y", L"ADIS Magno Z",
        L"ADIS AMANGLE", L"ADIS Magno Norm", L"MPU Magno Norm",
        L"ADIS Accel Norm", L"MPU Accel Norm"
    };

    auto window_size_node = std::make_shared<FTWindowSizeNode>();
    window_size_node->setAnchorPoint(glm::vec2(0, -1.0f));
    addChild(window_size_node);

    
	const float x_padding = 300;
    const float y_padding = 30.0f;
    float y = -y_padding;
	float x = 0;

    for (int i = 0; i < num_labels; i++) {
        auto label = std::make_shared<FTLabel>(label_names[i], 6);
        window_size_node->addChild(label);
        label->setPosition(glm::vec2(x + 30, y));
        label->setAnchorPoint(glm::vec2(0, 0.5f));

        label = std::make_shared<FTLabel>(L"0", 6, true);
        window_size_node->addChild(label);
        label->setPosition(glm::vec2(x + x_padding, y));
        label->setPosition(glm::vec2(x + x_padding, y));
        label->setAnchorPoint(glm::vec2(1, 0.5f));

        value_labels_.push_back(label.get());
        y -= y_padding;

        values[i] = 0;
		if (i % 25 == 24) {
			x += x_padding;
			y = -y_padding;
		}
    }

    messaging_consumer_init(&messaging_consumer);

    s_instance = this;

	FTEngine::getInputManager()->getKeyState("ToggleDetails", GLFW_KEY_SEMICOLON)->registerOnPressDelegate(this, &StateDetailView::toggleDetails);
}

StateDetailView::~StateDetailView() {
    FTLog("State Detail View Destroyed");
	FTEngine::getInputManager()->getKeyState("ToggleDetails", GLFW_KEY_SEMICOLON)->unregisterOnPressDelegate(this, &StateDetailView::toggleDetails);
}

void StateDetailView::updateDisplay(const FTUpdateEvent& event) {
    static float accumulator = 1.0f;
    accumulator += (float)event.delta_time_;
    if (accumulator >= 5.0) {
        accumulator -= 5.0f;
        values[13] = mpu9250_update_count / 5.0f;
        values[14] = state_estimate_update_count / 5.0f;
        values[15] = ms5611_update_count / 5.0f;
        mpu9250_update_count = 0;
        state_estimate_update_count = 0;
        ms5611_update_count = 0;
    }

    while (messaging_consumer_receive(&messaging_consumer, false, false) == messaging_receive_ok);

    static wchar_t buff[24];
    for (int i = 0; i < num_labels; i++)
        value_labels_[i]->setString(FTWCharUtil::formatString(buff, 24, L"%.2f", values[i]));
}

void StateDetailView::toggleDetails() {
	this->setHidden(!this->getHidden());
}

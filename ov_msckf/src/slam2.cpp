#include <functional>

#include <opencv/cv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <math.h>
#include <eigen3/Eigen/Dense>

#include "core/VioManager.h"
#include "state/State.h"

#include "common/plugin.hpp"
#include "common/switchboard.hpp"
#include "common/data_format.hpp"
#include "common/realtime_clock.hpp"
#include "common/phonebook.hpp"

using namespace ILLIXR;
using namespace ov_msckf;

// Comment in if using ZED instead of offline_imu_cam
// TODO: Pull from config YAML file
//#define ZED

std::string getenv_or(std::string var, std::string default_) {
	const char* val = std::getenv(var.c_str());
	return val ? std::string{val} : default_;
}

VioManagerOptions create_params()
{
	VioManagerOptions params;

	// Camera #0
	Eigen::Matrix<double, 8, 1> intrinsics_0;
#ifdef ZED
  // ZED calibration tool; fx, fy, cx, cy, k1, k2, p1, p2
  // https://docs.opencv.org/2.4/doc/tutorials/calib3d/camera_calibration/camera_calibration.html
  intrinsics_0 << 349.686, 349.686, 332.778, 192.423, -0.175708, 0.0284421, 0, 0;
#else
  // EuRoC
	intrinsics_0 << 458.654, 457.296, 367.215, 248.375, -0.28340811, 0.07395907, 0.00019359, 1.76187114e-05;
#endif

#ifdef ZED
  // Camera extrinsics from https://github.com/rpng/open_vins/issues/52#issuecomment-619480497
  std::vector<double> matrix_TCtoI_0 = {-0.01080233, 0.00183858, 0.99993996, 0.01220425,
            -0.99993288, -0.00420947, -0.01079452, 0.0146056,
            0.00418937, -0.99998945, 0.00188393, -0.00113692,
            0.0, 0.0, 0.0, 1.0};
#else
	std::vector<double> matrix_TCtoI_0 = {0.0148655429818, -0.999880929698, 0.00414029679422, -0.0216401454975,
            0.999557249008, 0.0149672133247, 0.025715529948, -0.064676986768,
            -0.0257744366974, 0.00375618835797, 0.999660727178, 0.00981073058949,
            0.0, 0.0, 0.0, 1.0};
#endif

  Eigen::Matrix4d T_CtoI_0;
	T_CtoI_0 << matrix_TCtoI_0.at(0), matrix_TCtoI_0.at(1), matrix_TCtoI_0.at(2), matrix_TCtoI_0.at(3),
		matrix_TCtoI_0.at(4), matrix_TCtoI_0.at(5), matrix_TCtoI_0.at(6), matrix_TCtoI_0.at(7),
		matrix_TCtoI_0.at(8), matrix_TCtoI_0.at(9), matrix_TCtoI_0.at(10), matrix_TCtoI_0.at(11),
		matrix_TCtoI_0.at(12), matrix_TCtoI_0.at(13), matrix_TCtoI_0.at(14), matrix_TCtoI_0.at(15);

	// Load these into our state
	Eigen::Matrix<double, 7, 1> extrinsics_0;
	extrinsics_0.block(0, 0, 4, 1) = rot_2_quat(T_CtoI_0.block(0, 0, 3, 3).transpose());
	extrinsics_0.block(4, 0, 3, 1) = -T_CtoI_0.block(0, 0, 3, 3).transpose() * T_CtoI_0.block(0, 3, 3, 1);

	params.camera_fisheye.insert({0, false});
	params.camera_intrinsics.insert({0, intrinsics_0});
	params.camera_extrinsics.insert({0, extrinsics_0});

#ifdef ZED
  params.camera_wh.insert({0, {672, 376}});
#else
	params.camera_wh.insert({0, {752, 480}});
#endif

	// Camera #1
	Eigen::Matrix<double, 8, 1> intrinsics_1;
#ifdef ZED
  // ZED calibration tool; fx, fy, cx, cy, k1, k2, p1, p2
  intrinsics_1 << 350.01, 350.01, 343.729, 185.405, -0.174559, 0.0277521, 0, 0;
#else
  // EuRoC
	intrinsics_1 << 457.587, 456.134, 379.999, 255.238, -0.28368365, 0.07451284, -0.00010473, -3.55590700e-05;
#endif

#ifdef ZED
  // Camera extrinsics from https://github.com/rpng/open_vins/issues/52#issuecomment-619480497
  std::vector<double> matrix_TCtoI_1 = {-0.01043535, -0.00191061, 0.99994372, 0.01190459,
            -0.99993668, -0.00419281, -0.01044329, -0.04732387,
            0.00421252, -0.99998938, -0.00186674, -0.00098799,
            0.0, 0.0, 0.0, 1.0};
#else
	std::vector<double> matrix_TCtoI_1 = {0.0125552670891, -0.999755099723, 0.0182237714554, -0.0198435579556,
            0.999598781151, 0.0130119051815, 0.0251588363115, 0.0453689425024,
            -0.0253898008918, 0.0179005838253, 0.999517347078, 0.00786212447038,
            0.0, 0.0, 0.0, 1.0};
#endif

  Eigen::Matrix4d T_CtoI_1;
	T_CtoI_1 << matrix_TCtoI_1.at(0), matrix_TCtoI_1.at(1), matrix_TCtoI_1.at(2), matrix_TCtoI_1.at(3),
		matrix_TCtoI_1.at(4), matrix_TCtoI_1.at(5), matrix_TCtoI_1.at(6), matrix_TCtoI_1.at(7),
		matrix_TCtoI_1.at(8), matrix_TCtoI_1.at(9), matrix_TCtoI_1.at(10), matrix_TCtoI_1.at(11),
		matrix_TCtoI_1.at(12), matrix_TCtoI_1.at(13), matrix_TCtoI_1.at(14), matrix_TCtoI_1.at(15);

	// Load these into our state
	Eigen::Matrix<double, 7, 1> extrinsics_1;
	extrinsics_1.block(0, 0, 4, 1) = rot_2_quat(T_CtoI_1.block(0, 0, 3, 3).transpose());
	extrinsics_1.block(4, 0, 3, 1) = -T_CtoI_1.block(0, 0, 3, 3).transpose() * T_CtoI_1.block(0, 3, 3, 1);

	params.camera_fisheye.insert({1, false});
	params.camera_intrinsics.insert({1, intrinsics_1});
	params.camera_extrinsics.insert({1, extrinsics_1});

#ifdef ZED
  params.camera_wh.insert({1, {672, 376}});
#else
	params.camera_wh.insert({1, {752, 480}});
#endif

	// params.state_options.max_slam_features = 0;
	params.state_options.num_cameras = 2;
	params.init_window_time = 0.75;
#ifdef ZED
  // Hand tuned
  params.init_imu_thresh = 0.5;
#else
  // EuRoC
	params.init_imu_thresh = 1.5;
#endif
	params.fast_threshold = 15;
	params.grid_x = 5;
	params.grid_y = 3;
#ifdef ZED
  // Hand tuned
  params.num_pts = 200;
#else
  unsigned num_pts = std::stoi(getenv_or("num_pts", "150"));
  params.num_pts = num_pts;
#endif
	params.msckf_options.chi2_multipler = 1;
	params.knn_ratio = .7;

	params.state_options.imu_avg = true;
	params.state_options.do_fej = true;
	int use_rk4_integration = std::stoi(getenv_or("use_rk4_integration", "1"));
	params.state_options.use_rk4_integration = !!use_rk4_integration;
	int use_stereo = std::stoi(getenv_or("use_stereo", "1"));
	params.use_stereo = !!use_stereo;
	params.state_options.do_calib_camera_pose = true;
	params.state_options.do_calib_camera_intrinsics = true;
	params.state_options.do_calib_camera_timeoffset = true;

	params.dt_slam_delay = 3.0;
	params.state_options.max_slam_features = 50;
	params.state_options.max_slam_in_update = 25;
	params.state_options.max_msckf_in_update = 999;

#ifdef ZED
  // Pixel noise; ZED works with defaults values but these may better account for rolling shutter
  params.msckf_options.chi2_multipler = 2;
  params.msckf_options.sigma_pix = 5;
	params.slam_options.chi2_multipler = 2;
	params.slam_options.sigma_pix = 5;

  // IMU biases from https://github.com/rpng/open_vins/issues/52#issuecomment-619480497
  params.imu_noises.sigma_a = 0.00395942;  // Accelerometer noise
  params.imu_noises.sigma_ab = 0.00072014; // Accelerometer random walk
  params.imu_noises.sigma_w = 0.00024213;  // Gyroscope noise
  params.imu_noises.sigma_wb = 1.9393e-05; // Gyroscope random walk
#else
	params.slam_options.chi2_multipler = 1;
	params.slam_options.sigma_pix = 1;
#endif

	params.use_aruco = false;

	params.state_options.feat_rep_slam = LandmarkRepresentation::from_string("ANCHORED_FULL_INVERSE_DEPTH");
  params.state_options.feat_rep_aruco = LandmarkRepresentation::from_string("ANCHORED_FULL_INVERSE_DEPTH");

  // int downsample_cameras = std::stoi(getenv_or("downsample_cameras", "0"));
  // params.downsample_cameras = !!downsample_cameras;

  // int try_zupt = std::stoi(getenv_or("try_zupt", "0"));
  // params.try_zupt = !!try_zupt;

  int use_klt = std::stoi(getenv_or("use_klt", "1"));
  params.use_klt = !!use_klt;

	return params;
}

class slam2 : public plugin {
public:
	/* Provide handles to slam2 */
	slam2(std::string name_, phonebook* pb_)
		: plugin{name_, pb_}
		, sb{pb->lookup_impl<switchboard>()}
		, _m_pose{sb->get_writer<pose_type>("slow_pose")}
		, _m_imu_integrator_input{sb->get_writer<imu_integrator_input>("imu_integrator_input")}
		, open_vins_estimator{manager_params}
		, _m_rtc{pb->lookup_impl<realtime_clock>()}
		// , _m_cam{sb->get_reader<cam_type>("cam")}
		, _m_cam{sb->get_buffered_reader<cam_type>("cam")}
	{
		_m_pose.put(new (_m_pose.allocate()) pose_type{
			_m_rtc->now(),
			Eigen::Vector3f{0, 0, 0},
			Eigen::Quaternionf{1, 0, 0, 0}
		});
	}


	virtual void start() override {
		plugin::start();
		auto& thread = sb->schedule<imu_type>(id, "imu", [&](switchboard::ptr<const imu_type> datum, std::size_t iteration_no) {
			this->feed_imu_cam(datum, iteration_no);
		});
		if (is_priority_scheduler()) {
			set_priority(thread.get_pid(), 2);
		}
	}

	void feed_imu_cam(switchboard::ptr<const imu_type> datum, std::size_t iteration_no) {

		auto start = std::chrono::steady_clock::now();
		auto start_comptime = cpu_timer::detail::cpu_now();

		// Ensures that slam doesnt start before valid IMU readings come in
		switchboard::ptr<const cam_type> cam;
       double timestamp_in_seconds = 0;
       {
	       CPU_TIMER_TIME_BLOCK("IMU");
		if (datum == nullptr) {
			std::cerr << "slam2:feed_imu_cam datum == nullptr\n";
			abort();
		}

		timestamp_in_seconds = (double(datum->dataset_time) / NANO_SEC.count());
		assert(timestamp_in_seconds > previous_timestamp);
		previous_timestamp = timestamp_in_seconds;

		// Feed the IMU measurement. There should always be IMU data in each call to feed_imu_cam
		open_vins_estimator.feed_measurement_imu(timestamp_in_seconds, (datum->angular_v).cast<double>(), (datum->linear_a).cast<double>());

		// std::cout << std::fixed << "Time of IMU/CAM: " << timestamp_in_seconds * 1e9 << " Lin a: " << 
		// 	datum->angular_v[0] << ", " << datum->angular_v[1] << ", " << datum->angular_v[2] << ", " <<
		// 	datum->linear_a[0] << ", " << datum->linear_a[1] << ", " << datum->linear_a[2] << std::endl;

		// Async:
		// cam = _m_cam.get_ro_nullable();
		cam = _m_cam.size() == 0 ? nullptr : _m_cam.dequeue();
		if (!cam) {
			return;
		}
		if (!cam_buffer) {
			cam_buffer = cam;
			return;
		}
		/* This is necsesary in async, to make sure that cam is new
		   In buffered, cam is new if it is present.
		   
		if (cam->time == cam_buffer->time) {
			return;
		}
		*/

	   }

		CPU_TIMER_TIME_BLOCK("cam");

		cv::Mat img0 = cv::Mat{cam_buffer->img0};
		cv::Mat img1 = cv::Mat{cam_buffer->img1};
		double buffer_timestamp_seconds = double(cam_buffer->dataset_time) / NANO_SEC.count();
		open_vins_estimator.feed_measurement_stereo(buffer_timestamp_seconds, img0, img1, 0, 1);

		// Get the pose returned from SLAM
		state = open_vins_estimator.get_state();
		Eigen::Vector4d quat = state->_imu->quat();
		Eigen::Vector3d vel = state->_imu->vel();
		Eigen::Vector3d pose = state->_imu->pos();

		Eigen::Vector3f swapped_pos = Eigen::Vector3f{float(pose(0)), float(pose(1)), float(pose(2))};
		Eigen::Quaternionf swapped_rot = Eigen::Quaternionf{float(quat(3)), float(quat(0)), float(quat(1)), float(quat(2))};
		Eigen::Quaterniond swapped_rot2 = Eigen::Quaterniond{(quat(3)), (quat(0)), (quat(1)), (quat(2))};

       	assert(isfinite(swapped_rot.w()));
        assert(isfinite(swapped_rot.x()));
        assert(isfinite(swapped_rot.y()));
        assert(isfinite(swapped_rot.z()));
        assert(isfinite(swapped_pos[0]));
        assert(isfinite(swapped_pos[1]));
        assert(isfinite(swapped_pos[2]));

		if (open_vins_estimator.initialized()) {
			CPU_TIMER_TIME_BLOCK("publish");

			if (isUninitialized) {
				isUninitialized = false;
			}

			_m_pose.put(new (_m_pose.allocate()) pose_type{
				cam_buffer->time,
				swapped_pos,
				swapped_rot,
			});

			_m_imu_integrator_input.put(new (_m_imu_integrator_input.allocate()) imu_integrator_input{
				timestamp_in_seconds,
				state->_calib_dt_CAMtoIMU->value()(0),
				{
					.gyro_noise = 0.00016968,
					.acc_noise = 0.002,
					.gyro_walk = 1.9393e-05,
					.acc_walk = 0.003,
					.n_gravity = Eigen::Matrix<double,3,1>(0.0, 0.0, -9.81),
					.imu_integration_sigma = 1.0,
					.nominal_rate = 200.0,
				},
				state->_imu->bias_a(),
				state->_imu->bias_g(),
				pose,
				vel,
				swapped_rot2,
			});
		}

		cam_buffer = cam;

		auto stop = std::chrono::steady_clock::now();
		auto stop_comptime = cpu_timer::detail::cpu_now();
		if (stop - start > std::chrono::milliseconds{100}) {
			std::cerr << "\e[1;34mSLAM is slow. Ratio = " << slow_count << ":" << fast_count << ", wall time = " << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << "ms, comp time = " << std::chrono::duration_cast<std::chrono::milliseconds>(stop_comptime - start_comptime).count() << "ms, timestamp = epoch+" << std::chrono::duration_cast<std::chrono::milliseconds>(start.time_since_epoch()).count() << "ms\e[0m\n";
			slow_count++;
		} else {
			fast_count++;
		}
	}

	size_t slow_count = 0, fast_count = 0;

private:
	const std::shared_ptr<switchboard> sb;
	switchboard::writer<pose_type> _m_pose;
	switchboard::writer<imu_integrator_input> _m_imu_integrator_input;

	time_point _m_begin;
	State *state;

	VioManagerOptions manager_params = create_params();
	VioManager open_vins_estimator;

	switchboard::ptr<const cam_type> cam_buffer;
	double previous_timestamp = 0.0;
	bool isUninitialized = true;
	std::shared_ptr<realtime_clock> _m_rtc;
	// switchboard::reader<cam_type> _m_cam;
	switchboard::buffered_reader<cam_type> _m_cam;

};

PLUGIN_MAIN(slam2)

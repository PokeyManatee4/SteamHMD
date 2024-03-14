#include "hmd_device_driver.h"

#include "driverlog.h"
#include "vrmath.h"
#include <string.h>

// Let's create some variables for strings used in getting settings.
// This is the section where all of the settings we want are stored. A section name can be anything,
// but if you want to store driver specific settings, it's best to namespace the section with the driver identifier
// ie "<my_driver>_<section>" to avoid collisions
static const char *my_hmd_main_settings_section = "SteamHMD";
static const char *my_hmd_display_settings_section = "Android-Display";

MyHMDControllerDeviceDriver::MyHMDControllerDeviceDriver()
{
	is_active_ = false;
	char model_number[ 1024 ];
	vr::VRSettings()->GetString( my_hmd_main_settings_section, "model_number", model_number, sizeof( model_number ) );
	my_hmd_model_number_ = model_number;
	char serial_number[ 1024 ];
	vr::VRSettings()->GetString( my_hmd_main_settings_section, "serial_number", serial_number, sizeof( serial_number ) );
	my_hmd_serial_number_ = serial_number;
	MyHMDDisplayDriverConfiguration display_configuration{};
	display_configuration.window_x = vr::VRSettings()->GetInt32( my_hmd_display_settings_section, "window_x" );
	display_configuration.window_y = vr::VRSettings()->GetInt32( my_hmd_display_settings_section, "window_y" );
	display_configuration.window_width = vr::VRSettings()->GetInt32( my_hmd_display_settings_section, "window_width" );
	display_configuration.window_height = vr::VRSettings()->GetInt32( my_hmd_display_settings_section, "window_height" );
	display_configuration.render_width = vr::VRSettings()->GetInt32( my_hmd_display_settings_section, "render_width" );
	display_configuration.render_height = vr::VRSettings()->GetInt32( my_hmd_display_settings_section, "render_height" );
	my_display_component_ = std::make_unique< MyHMDDisplayComponent >( display_configuration );
}

vr::EVRInitError MyHMDControllerDeviceDriver::Activate( uint32_t unObjectId )
{
	device_index_ = unObjectId;
	is_active_ = true;
	frame_number_ = 0;
	vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer( device_index_ );
	vr::VRProperties()->SetStringProperty( container, vr::Prop_ModelNumber_String, my_hmd_model_number_.c_str() );
	const float ipd = vr::VRSettings()->GetFloat( vr::k_pch_SteamVR_Section, vr::k_pch_SteamVR_IPD_Float );
	vr::VRProperties()->SetFloatProperty( container, vr::Prop_UserIpdMeters_Float, ipd );
	vr::VRProperties()->SetFloatProperty( container, vr::Prop_DisplayFrequency_Float, 0.f );
	vr::VRProperties()->SetFloatProperty( container, vr::Prop_UserHeadToEyeDepthMeters_Float, 0.f );
	vr::VRProperties()->SetFloatProperty( container, vr::Prop_SecondsFromVsyncToPhotons_Float, 0.11f );
	vr::VRProperties()->SetBoolProperty( container, vr::Prop_IsOnDesktop_Bool, false );
	vr::VRProperties()->SetBoolProperty(container, vr::Prop_DisplayDebugMode_Bool, true);
	vr::VRProperties()->SetStringProperty( container, vr::Prop_InputProfilePath_String, "{simplehmd}/input/mysimplehmd_profile.json" );
	vr::VRDriverInput()->CreateBooleanComponent( container, "/input/system/touch", &my_input_handles_[ MyComponent_system_touch ] );
	vr::VRDriverInput()->CreateBooleanComponent( container, "/input/system/click", &my_input_handles_[ MyComponent_system_click ] );

	my_pose_update_thread_ = std::thread( &MyHMDControllerDeviceDriver::MyPoseUpdateThread, this );

	return vr::VRInitError_None;
}

void *MyHMDControllerDeviceDriver::GetComponent( const char *pchComponentNameAndVersion )
{
	if ( strcmp( pchComponentNameAndVersion, vr::IVRDisplayComponent_Version ) == 0 )
	{
		return my_display_component_.get();
	}

	return nullptr;
}

void MyHMDControllerDeviceDriver::DebugRequest( const char *pchRequest, char *pchResponseBuffer, uint32_t unResponseBufferSize )
{
	if ( unResponseBufferSize >= 1 )
		pchResponseBuffer[ 0 ] = 0;
}

vr::DriverPose_t MyHMDControllerDeviceDriver::GetPose()
{vr::DriverPose_t pose = { 0 };

	pose.qWorldFromDriverRotation.w = 1.f;
	pose.qDriverFromHeadRotation.w = 1.f;

	pose.qRotation.w = 1.f;

	pose.vecPosition[ 0 ] = 0.0f; // X axis
	pose.vecPosition[ 1 ] = sin( frame_number_ * 0.01 ) * 0.1f + 1.0f; // y axix
	pose.vecPosition[ 2 ] = 0.0f; // z axis

	pose.poseIsValid = true;

	pose.deviceIsConnected = true;

	pose.result = vr::TrackingResult_Running_OK;

	pose.shouldApplyHeadModel = true;

	return pose;
}

void MyHMDControllerDeviceDriver::MyPoseUpdateThread()
{
	while ( is_active_ )
	{
		vr::VRServerDriverHost()->TrackedDevicePoseUpdated( device_index_, GetPose(), sizeof( vr::DriverPose_t ) );

		std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
	}
}

void MyHMDControllerDeviceDriver::EnterStandby()
{
	DriverLog( "HMD has been put into standby." );
}

void MyHMDControllerDeviceDriver::Deactivate()
{
	if ( is_active_.exchange( false ) )
	{
		my_pose_update_thread_.join();
	}

	device_index_ = vr::k_unTrackedDeviceIndexInvalid;
}

void MyHMDControllerDeviceDriver::MyRunFrame()
{
	frame_number_++;
}

void MyHMDControllerDeviceDriver::MyProcessEvent( const vr::VREvent_t &vrevent )
{
}


const std::string &MyHMDControllerDeviceDriver::MyGetSerialNumber()
{
	return my_hmd_serial_number_;
}


MyHMDDisplayComponent::MyHMDDisplayComponent( const MyHMDDisplayDriverConfiguration &config )
	: config_( config )
{
}

bool MyHMDDisplayComponent::IsDisplayOnDesktop()
{
	return true;
}
bool MyHMDDisplayComponent::IsDisplayRealDisplay()
{
	return false;
}

void MyHMDDisplayComponent::GetRecommendedRenderTargetSize( uint32_t *pnWidth, uint32_t *pnHeight )
{
	*pnWidth = config_.render_width;
	*pnHeight = config_.render_height;
}

void MyHMDDisplayComponent::GetEyeOutputViewport( vr::EVREye eEye, uint32_t *pnX, uint32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight )
{
	*pnY = 0;

	*pnWidth = config_.window_width / 2;

	*pnHeight = config_.window_height;

	if ( eEye == vr::Eye_Left )
	{
		*pnX = 0;
	}
	else
	{
		*pnX = config_.window_width / 2;
	}
}

void MyHMDDisplayComponent::GetProjectionRaw( vr::EVREye eEye, float *pfLeft, float *pfRight, float *pfTop, float *pfBottom )
{
	*pfLeft = -1.0;
	*pfRight = 1.0;
	*pfTop = -1.0;
	*pfBottom = 1.0;
}

vr::DistortionCoordinates_t MyHMDDisplayComponent::ComputeDistortion( vr::EVREye eEye, float fU, float fV )
{
	vr::DistortionCoordinates_t coordinates{};
	coordinates.rfBlue[ 0 ] = fU;
	coordinates.rfBlue[ 1 ] = fV;
	coordinates.rfGreen[ 0 ] = fU;
	coordinates.rfGreen[ 1 ] = fV;
	coordinates.rfRed[ 0 ] = fU;
	coordinates.rfRed[ 1 ] = fV;
	return coordinates;
}

void MyHMDDisplayComponent::GetWindowBounds( int32_t *pnX, int32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight )
{
	*pnX = config_.window_x;
	*pnY = config_.window_y;
	*pnWidth = config_.window_width;
	*pnHeight = config_.window_height;
}
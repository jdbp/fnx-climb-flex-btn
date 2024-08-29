#include <MSFS\MSFS.h>
#include <MSFS\MSFS_WindowsTypes.h>
#include <SimConnect.h>
#include <string>

#include "Module.h"

#define FLEX_THROTTLE 4.0
#define CLIMB_THROTTLE 3.0

HANDLE g_hSimConnect;
const char* FCButtonEventPrefix = "FCBtn.";

static enum DATA_DEFINE_ID {
	DEFINITION_1,
};
static enum DATA_REQUEST_ID {
	REQUEST_1,
};
static enum EVENT_ID {
	EVENT_1,
};
static enum GROUP_ID {
	GROUP_1,
};
struct Struct1
{
	double throttleL;
	double throttleR;
	double chrono;
};


double currentChrono = 0.0;

void CALLBACK MyDispatchProc(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext);

extern "C" MSFS_CALLBACK void module_init(void)
{

	g_hSimConnect = 0;
	HRESULT hr = SimConnect_Open(&g_hSimConnect, "fnx-flex-climb-button", 0, 0, 0, 0);
	if (hr != S_OK)
	{
		fprintf(stderr, "Could not open SimConnect connection.\n");
		return;
	}

	// FNX
	hr = SimConnect_AddToDataDefinition(g_hSimConnect, DEFINITION_1, "L:A_FC_THROTTLE_LEFT_INPUT", "number", SIMCONNECT_DATATYPE_FLOAT64);
	hr = SimConnect_AddToDataDefinition(g_hSimConnect, DEFINITION_1, "L:A_FC_THROTTLE_RIGHT_INPUT", "number", SIMCONNECT_DATATYPE_FLOAT64);
	hr = SimConnect_AddToDataDefinition(g_hSimConnect, DEFINITION_1, "L:S_MIP_CHRONO_CAPT", "number", SIMCONNECT_DATATYPE_FLOAT64);

	hr = SimConnect_RequestDataOnSimObject(g_hSimConnect, REQUEST_1, DEFINITION_1, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_SIM_FRAME, SIMCONNECT_DATA_REQUEST_FLAG_CHANGED);


	// FBW
	std::string eventName(FCButtonEventPrefix, strlen(FCButtonEventPrefix));
	eventName += "A32NX_EFIS_L_CHRONO_PUSHED";
	hr = SimConnect_MapClientEventToSimEvent(g_hSimConnect, EVENT_1, eventName.c_str());

	hr = SimConnect_AddClientEventToNotificationGroup(g_hSimConnect, GROUP_1, EVENT_1, false);
	SimConnect_SetNotificationGroupPriority(g_hSimConnect, GROUP_1, SIMCONNECT_GROUP_PRIORITY_HIGHEST);

	hr = SimConnect_CallDispatch(g_hSimConnect, MyDispatchProc, 0);
	if (hr != S_OK)
	{
		fprintf(stderr, "Could not set dispatch proc.\n");
		return;
	}

}

extern "C" MSFS_CALLBACK void module_deinit(void)
{

	if (!g_hSimConnect)
		return;
	HRESULT hr = SimConnect_Close(g_hSimConnect);
	if (hr != S_OK)
	{
		fprintf(stderr, "Could not close SimConnect connection.\n");
		return;
	}

}

void setFnxThrottles(double left, double right)
{
	if (left > 2.1 && left < 2.9) {
		double throttle = FLEX_THROTTLE;
		double values[3] = { throttle, throttle, currentChrono };
		SimConnect_SetDataOnSimObject(g_hSimConnect, DEFINITION_1, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(values), &values[0]);
		//fprintf(stderr, "FLEX\n");
	}
	else if (left > 3.2 && right > 3.2) {
		double throttle = CLIMB_THROTTLE;
		double values[3] = { throttle, throttle, currentChrono };
		SimConnect_SetDataOnSimObject(g_hSimConnect, DEFINITION_1, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(values), &values[0]);
		//fprintf(stderr, "CLIMB\n");
	}
}

void CALLBACK MyDispatchProc(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext)
{
	switch (pData->dwID)
	{
	case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
	{
		SIMCONNECT_RECV_SIMOBJECT_DATA* pObjData = (SIMCONNECT_RECV_SIMOBJECT_DATA*)pData;
		switch (pObjData->dwRequestID)
		{
		case REQUEST_1:
		{
			Struct1* pS = (Struct1*)&pObjData->dwData;

			// set fnx throttle values
			if (pS->chrono != currentChrono) {
				currentChrono = pS->chrono;
				//fprintf(stderr, "Chrono button pushed! Value: %f\n", currentChrono);
				int intValue = static_cast<int>(currentChrono);
				if (intValue % 2 != 0) {
					setFnxThrottles(pS->throttleL, pS->throttleR);
				}
			}
			break;
		}
		}
		break;
	}
	case SIMCONNECT_RECV_ID_EVENT:
	{
		SIMCONNECT_RECV_EVENT* pEvent = (SIMCONNECT_RECV_EVENT*)pData;
		switch (pEvent->uEventID)
		{
		case EVENT_1:
			//fprintf(stderr, "L_CHRONO_PUSHED event received\n");
			break;
		}
		break;
	}
	default:
		break;
	}
}


#include "WholeFlexFeet.hh"
#include "FlexProxFoot.hh"
#include "FlexDistFoot.hh"
#include "SimuIndex.hh"

#define NB_BODIES 4

enum{R_FLEX_PROX_ID, R_FLEX_DIST_ID, L_FLEX_PROX_ID, L_FLEX_DIST_ID, NB_FEET_BODIES};


/*! \brief contructor
 * 
 * \param[in] mbs_data Robotran structure
 * \param[in] gait_features features of the gait
 * \param[in] sens_info information from sensors
 * \param[in] ground ground model
 */
WholeFlexFeet::WholeFlexFeet(MbsData *mbs_data, GaitFeatures *gait_features, SensorsInfo *sens_info, GroundModel *ground): WholeFeet(mbs_data, gait_features, sens_info, ground)
{
	for(int i=0; i<NB_FEET_BODIES; i++)
	{
		switch (i)
		{
			case R_FLEX_PROX_ID : bodies.push_back(new FlexProxFoot(mbs_data, gait_features, sens_info, R_FOOT_ID)); break;
			case R_FLEX_DIST_ID : bodies.push_back(new FlexDistFoot(mbs_data, gait_features, sens_info, R_FOOT_ID)); break;
			case L_FLEX_PROX_ID : bodies.push_back(new FlexProxFoot(mbs_data, gait_features, sens_info, L_FOOT_ID ));  break;
			case L_FLEX_DIST_ID : bodies.push_back(new FlexDistFoot(mbs_data, gait_features, sens_info, L_FOOT_ID ));  break;
		
			default: break;
		}
	}
}

/*! \brief destructor
 */
WholeFlexFeet::~WholeFlexFeet()
{
	// delete already done in mother class
}

/*! \brief compute the external ground reactions related to this foot
 * 
 * \param[out] F_tot external force output [N]
 * \param[out] T_tot external torque output [Nm]
 * \param[in] PxF absolute position (provided by Robotran) [m]
 * \param[in] RxF absolute rotation matrix (provided by Robotran) [-]
 * \param[in] VxF absolute position derivative (provided by Robotran) [m/s]
 * \param[in] OMxF absolute rotation derivatives (provided by Robotran) [rad/s]
 * \param[in] index index of the external sensor
 */
void WholeFlexFeet::compute_F_T(double F_tot[3], double T_tot[3], double PxF[4], double RxF[4][4], double VxF[4], double OMxF[4], int index)
{
	ContactFoot *body;

	switch ( index ) 
	{
		case SimuFsensIndex::RightFlexProx:
			body = bodies[R_FLEX_PROX_ID];
			break;

		case SimuFsensIndex::LeftFlexProx:
			body = bodies[L_FLEX_PROX_ID];
			break;

		case SimuFsensIndex::RightFlexDist:
			body = bodies[R_FLEX_DIST_ID];
			break;

		case SimuFsensIndex::LeftFlexDist:
			body = bodies[L_FLEX_DIST_ID];
			break;

		default:
			body = NULL;
			break;
	}

	body->update_pose(PxF, RxF, VxF, OMxF);
	
	ground->compute_F_T(F_tot, T_tot, body);
}

/*! \brief get feet forces
 * 
 * \param[in] leg_id ID of the leg
 * \param[in] axis axis requested
 * \return force requested [N]
 */
double WholeFlexFeet::get_leg_feet_forces(int leg_id, int axis)
{
	switch (leg_id)
	{
		case RIGHT_ID:
			return bodies[R_FLEX_PROX_ID]->get_F_tot(axis) + bodies[R_FLEX_DIST_ID]->get_F_tot(axis);
			break;

		case LEFT_ID:
			return bodies[L_FLEX_PROX_ID]->get_F_tot(axis) + bodies[L_FLEX_DIST_ID]->get_F_tot(axis);
			break;
	
		default:
			return 0.0;
			break;
	}
}

/*! \brief get feet torques
 * 
 * \param[in] leg_id ID of the leg
 * \param[in] axis axis requested
 * \return torque requested [Nm]
 */
double WholeFlexFeet::get_leg_feet_torques(int leg_id, int axis)
{
	switch (leg_id)
	{
		case RIGHT_ID:
			return bodies[R_FLEX_PROX_ID]->get_T_tot(axis) + bodies[R_FLEX_DIST_ID]->get_T_tot(axis);
			break;

		case LEFT_ID:
			return bodies[L_FLEX_PROX_ID]->get_T_tot(axis) + bodies[L_FLEX_DIST_ID]->get_T_tot(axis);
			break;
	
		default:
			return 0.0;
			break;
	}
}

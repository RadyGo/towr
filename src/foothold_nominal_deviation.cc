/**
 @file    foothold_nominal_deviation.cc
 @author  Alexander W. Winkler (winklera@ethz.ch)
 @date    May 30, 2016
 @brief   Brief description
 */

#include <xpp/zmp/foothold_nominal_deviation.h>

namespace xpp {
namespace zmp {

FootholdNominalDeviation::FootholdNominalDeviation ()
{
  // TODO Auto-generated constructor stub
}

FootholdNominalDeviation::VectorXd
FootholdNominalDeviation::DistanceToNominalStance (
    const ContinuousSplineContainer& cog_spline,
    const SupportPolygonContainer& supp_polygon_container) const
{
  const double x_nominal_b = 0.3; // 0.4
  const double y_nominal_b = 0.3; // 0.4

  xpp::hyq::LegDataMap<Eigen::Vector2d> B_r_BaseToNominal;
  B_r_BaseToNominal[hyq::LF] <<  x_nominal_b,  y_nominal_b;
  B_r_BaseToNominal[hyq::RF] <<  x_nominal_b, -y_nominal_b;
  B_r_BaseToNominal[hyq::LH] << -x_nominal_b,  y_nominal_b;
  B_r_BaseToNominal[hyq::RH] << -x_nominal_b, -y_nominal_b;

  int N     = cog_spline.GetTotalNodes();
  int approx_n_constraints = 4*N*2; // 3 or 4 legs in contact at every discrete time, 2 b/c x-y
  std::vector<double> g_vec;
  g_vec.reserve(approx_n_constraints);

  std::vector<xpp::hyq::SupportPolygon> suppport_polygons =
      supp_polygon_container.CreateSupportPolygonsWith4LS(cog_spline.GetSplines());

  double T = cog_spline.GetDiscretizedGlobalTimes().back();

  for (double t : cog_spline.GetDiscretizedGlobalTimes()) {

    // get legs in contact at each step
    VecFoothold stance_legs;
    int id = cog_spline.GetSplineID(t);

    // final foothold never creates aktive support polygon, so handle manually
    if (t == T)
      stance_legs = supp_polygon_container.GetFinalFootholds();
    else
      stance_legs = suppport_polygons.at(id).footholds_;

    xpp::utils::Point2d cog_xy = cog_spline.GetCOGxy(t);

    // calculate distance to base for every stance leg
    // restrict to quadrants
    for (const hyq::Foothold& f : stance_legs) {

      // base to foot
      Eigen::Vector2d r_BF = f.p.segment<2>(0) - cog_xy.p;

      // current foot to nominal
      Eigen::Vector2d r_FC = -r_BF + B_r_BaseToNominal[f.leg];

      // don't use std::fabs() or std::pow() since you loose information about sign
      // this information help the optimizer to find the correct gradient.
      g_vec.push_back(r_FC.x());
      g_vec.push_back(r_FC.y());
    }
  }

  return Eigen::Map<const VectorXd>(&g_vec.front(),g_vec.size());
}

} /* namespace zmp */
} /* namespace xpp */

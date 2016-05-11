#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <algorithm>
#include <iostream>
#include <string>
#include <cmath>
#include <deque>
#include <map>
#include <set>

namespace config {

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 860;
const double LAPTOP_DISTANCE_M = 6.0;
const double PIXEL_DISTANCE = 200;
const double AVERAGE_TIME_WINDOW_MS = 3000.0;

}

namespace utils {

static double LAST_TS;

struct Circle {
  
  Circle() {}
  Circle(int x, int y, int rad, int color) : x(x), y(y), rad(rad), color(color) {}

  int x;
  int y;
  int rad;
  unsigned int color;
};

static void intersect(double r0, double r1, double *dx, double *dy) {
  std::cerr << "Estimating position intersection for r0: " << r0 << " r1: " << r1 << std::endl;
  double l = config::LAPTOP_DISTANCE_M;
  if (l > r0 + r1) {
    double dr = (l - r0 - r1)/2.0;
    r0 += dr;
    r1 += dr;
  } else if (r0 > l + r1) {
    double dr = (r0 - l - r1)/2.0;
    r0 -= dr;
    r1 += dr;
  } else if (r1 > l + r0) {
    double dr = (r1 - l - r0)/2.0;
    r0 += dr;
    r1 -= dr;
  }

  double A = 0.0, B = 2 * l, C = l*l + r0*r0 - r1*r1;
  double d = sqrt(std::max(0.0, r0*r0 - C*C/(A*A+B*B)));
  double mult = sqrt(d*d / (A*A + B*B));

  *dx = 0.0 + B * mult;
  *dy = -B*C/(A*A + B*B) - A*mult + l/2.0;
  std::cerr << "Estimated: " << *dx << " " << *dy << " r0: " << r0 << " r1: " << r1 << std::endl;
}

struct Notification {

  Notification() {}
  Notification(double val, double ts) : val(val), ts(ts) {}

  double val;
  double ts;

};

class TimeAverager {

 public:

  TimeAverager() : _mapping(), _tot_sum() {} 

  void add(const std::string &mac, const Notification &notif) {
    _mapping[mac].push_back(notif);
    _tot_sum[mac] += notif.val;
    relax(mac);
  }

  bool get(const std::string &mac, double *val) {
    relax(mac);
    if (!_mapping.count(mac) || _mapping[mac].empty()) {
      return false;
    }
    *val = _tot_sum[mac] / _mapping[mac].size();
    return true;
  }

  std::set<std::string> keys() const {
    std::set<std::string> result;
    for (const auto &kv : _mapping) {
      result.insert(kv.first);
    }
    return result;
  }

 private:

  void relax(const std::string &mac) {
    for (; !_mapping[mac].empty();) {
      const Notification notif = _mapping[mac].front();
      if (LAST_TS - notif.ts > config::AVERAGE_TIME_WINDOW_MS) {
        _tot_sum[mac] -= notif.val;
        _mapping[mac].pop_front();
      } else {
        break;
      }
    }
  }

 private:

  std::map<std::string, std::deque<utils::Notification>> _mapping;
  std::map<std::string, double> _tot_sum;

};


static void CalculatePositionEstimates(TimeAverager R[2], TimeAverager *X, TimeAverager *Y, const double ts) {
  std::set<std::string> macs = R[0].keys();
  for (const auto &mac : macs) {
    // Ensure that we have measurements from both to proceed.
    double r0, r1;
    double dx, dy;
    // Drop unnecessary packets.
    X->get(mac, &dx);
    Y->get(mac, &dy);
    if (!R[0].get(mac, &r0)) {
      continue;
    }
    if (!R[1].get(mac, &r1)) {
      continue;
    }

    // Update position estimates.
    intersect(r0, r1, &dx, &dy);
    X->add(mac, Notification(dx, ts));
    Y->add(mac, Notification(dy, ts));
  }
}

} // namespace utils

#endif // __UTILS_HPP__

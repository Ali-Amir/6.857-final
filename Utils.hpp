#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <algorithm>
#include <string>
#include <cmath>
#include <deque>
#include <map>
#include <set>

namespace config {

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 860;
const double LAPTOP_DISTANCE_M = 2.5;
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
  int color;
};

static void intersect(double r0, double r1, double *dx, double *dy) {
  double l = config::LAPTOP_DISTANCE_M;
  double add = std::max(0.0, (l - r0 - r1)/2.0);
  r0 += add;
  r1 += add;

  *dx = sqrt(std::max(0.0, std::pow(r0, 2.0) - std::pow(std::pow(r1, 2.0) - std::pow(r0, 2.0) - std::pow(l, 2.0), 2.0) / std::pow(2*l, 2.0)));
  *dy = l/2.0 - sqrt(std::pow(r1, 2.0) - std::pow(*dx, 2.0));
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
    if (_mapping.count(mac)) {
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
    if (!R[0].get(mac, &r0)) {
      continue;
    }
    if (!R[1].get(mac, &r1)) {
      continue;
    }

    // Update position estimates.
    double dx, dy;
    intersect(r0, r1, &dx, &dy);
    X->add(mac, Notification(dx, ts));
    Y->add(mac, Notification(dy, ts));
  }
}

} // namespace utils

#endif // __UTILS_HPP__

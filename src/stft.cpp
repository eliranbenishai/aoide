#include "stft.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace aoide {
namespace {

constexpr double kPi = 3.14159265358979323846;

std::vector<double> hann(int n) {
  std::vector<double> w(size_t(n), 0.0);
  for (int i = 0; i < n; ++i) {
    w[size_t(i)] = 0.5 * (1.0 - std::cos(2.0 * kPi * double(i) / double(n - 1)));
  }
  return w;
}

std::vector<double> logBandEdgeHz(int sampleRateHz) {
  constexpr double fMin = 40.0;
  const double fMax = sampleRateHz / 2.0;
  std::vector<double> edges;
  edges.reserve(AudioLevels::kBandCount + 1);
  for (int i = 0; i <= AudioLevels::kBandCount; ++i) {
    const double t = double(i) / double(AudioLevels::kBandCount);
    edges.push_back(fMin * std::pow(fMax / fMin, t));
  }
  return edges;
}

std::vector<int> logBandEdges(int sampleRateHz, int fftSize) {
  const std::vector<double> edgesHz = logBandEdgeHz(sampleRateHz);
  const double binHz = double(sampleRateHz) / double(fftSize);
  const int maxBin = (fftSize / 2) - 1;
  std::vector<int> bins;
  bins.reserve(edgesHz.size());
  for (double hz : edgesHz) {
    int bin = int(std::floor(hz / binHz));
    bins.push_back(std::clamp(bin, 1, maxBin));
  }
  // Independent clamping collapses the lowest log edges onto FFT bin 1 when
  // bin width (~43 Hz at 1024/44.1 kHz) is wider than the first bands, which
  // leaves bars 0–1 with lo==hi and dumps all bass into bar 2. Keep edges
  // strictly increasing so every bar owns at least one bin.
  for (size_t i = 1; i < bins.size(); ++i) {
    if (bins[i] <= bins[i - 1]) {
      bins[i] = bins[i - 1] + 1;
    }
  }
  if (bins.back() > maxBin) {
    bins.back() = maxBin;
    for (int i = int(bins.size()) - 2; i >= 0; --i) {
      if (bins[size_t(i)] >= bins[size_t(i + 1)]) {
        bins[size_t(i)] = bins[size_t(i + 1)] - 1;
      }
    }
  }
  return bins;
}

void fftInPlace(std::vector<double>& re, std::vector<double>& im) {
  const int n = int(re.size());
  if (n == 0 || (n & (n - 1)) != 0) {
    throw std::invalid_argument("fftSize must be power of two");
  }

  int j = 0;
  for (int i = 1; i < n; ++i) {
    int bit = n >> 1;
    for (; j & bit; bit >>= 1) {
      j ^= bit;
    }
    j ^= bit;
    if (i < j) {
      std::swap(re[size_t(i)], re[size_t(j)]);
      std::swap(im[size_t(i)], im[size_t(j)]);
    }
  }

  for (int len = 2; len <= n; len <<= 1) {
    const double ang = -2.0 * kPi / double(len);
    const double wlenRe = std::cos(ang);
    const double wlenIm = std::sin(ang);
    for (int i = 0; i < n; i += len) {
      double wRe = 1.0;
      double wIm = 0.0;
      for (int k = 0; k < len / 2; ++k) {
        const double uRe = re[size_t(i + k)];
        const double uIm = im[size_t(i + k)];
        const double vRe = re[size_t(i + k + len / 2)] * wRe - im[size_t(i + k + len / 2)] * wIm;
        const double vIm = re[size_t(i + k + len / 2)] * wIm + im[size_t(i + k + len / 2)] * wRe;
        re[size_t(i + k)] = uRe + vRe;
        im[size_t(i + k)] = uIm + vIm;
        re[size_t(i + k + len / 2)] = uRe - vRe;
        im[size_t(i + k + len / 2)] = uIm - vIm;
        const double nextWRe = wRe * wlenRe - wIm * wlenIm;
        wIm = wRe * wlenIm + wIm * wlenRe;
        wRe = nextWRe;
      }
    }
  }
}

}  // namespace

StftBandFolder::StftBandFolder(int fftSize, int framesPerSecond)
    : fftSize_(fftSize), framesPerSecond_(framesPerSecond) {}

QVector<std::array<double, AudioLevels::kBandCount>> StftBandFolder::analyze(
    const QVector<double>& samples, int sampleRateHz) const {
  QVector<std::array<double, AudioLevels::kBandCount>> frames;
  if (samples.isEmpty() || sampleRateHz <= 0) {
    frames.push_back({});
    return frames;
  }

  const int hop = std::max(1, sampleRateHz / framesPerSecond_);
  const std::vector<double> window = hann(fftSize_);
  const std::vector<int> bandEdges = logBandEdges(sampleRateHz, fftSize_);
  const int magCount = fftSize_ / 2;

  for (int start = 0; start + fftSize_ <= samples.size(); start += hop) {
    std::vector<double> re(size_t(fftSize_), 0.0);
    std::vector<double> im(size_t(fftSize_), 0.0);
    for (int i = 0; i < fftSize_; ++i) {
      re[size_t(i)] = samples[start + i] * window[size_t(i)];
    }
    fftInPlace(re, im);

    std::vector<double> mags(size_t(magCount), 0.0);
    for (int k = 0; k < magCount; ++k) {
      mags[size_t(k)] = std::sqrt(re[size_t(k)] * re[size_t(k)] + im[size_t(k)] * im[size_t(k)]);
    }

    std::array<double, AudioLevels::kBandCount> bands{};
    for (int b = 0; b < AudioLevels::kBandCount; ++b) {
      const int lo = bandEdges[size_t(b)];
      const int hi = bandEdges[size_t(b + 1)];
      double sum = 0;
      int count = 0;
      for (int k = lo; k < hi && k < magCount; ++k) {
        sum += mags[size_t(k)];
        ++count;
      }
      bands[size_t(b)] = count == 0 ? 0.0 : sum / double(count);
    }

    double peak = 0;
    for (double v : bands) {
      if (v > peak) peak = v;
    }
    if (peak > 1e-12) {
      for (double& v : bands) {
        v = std::clamp(v / peak, 0.0, 1.0);
      }
    }
    frames.push_back(bands);
  }

  if (frames.isEmpty()) {
    frames.push_back({});
  }
  return frames;
}

double StftBandFolder::bandCenterHz(int index, int sampleRateHz) {
  const std::vector<double> edgesHz = logBandEdgeHz(sampleRateHz);
  return std::sqrt(edgesHz[size_t(index)] * edgesHz[size_t(index + 1)]);
}

}  // namespace aoide

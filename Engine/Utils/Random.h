#pragma once
#include <random>

namespace gns
{
	class Random
	{
		template <typename I, typename = std::enable_if_t <
			std::is_same_v<I, uint16_t> ||
			std::is_same_v<I, int16_t> ||
			std::is_same_v<I, uint32_t> ||
			std::is_same_v<I, int32_t> ||
			std::is_same_v<I, uint64_t> ||
			std::is_same_v<I, int64_t>>>
			static I Generate_int(std::mt19937& engine) {
			std::uniform_int_distribution<I> dis(
				std::numeric_limits<I>::min(),
				std::numeric_limits<I>::max()
			);
			return dis(engine);
		}
		template <typename I, typename = std::enable_if_t <
			std::is_same_v<I, uint16_t> ||
			std::is_same_v<I, int16_t> ||
			std::is_same_v<I, uint32_t> ||
			std::is_same_v<I, int32_t> ||
			std::is_same_v<I, uint64_t> ||
			std::is_same_v<I, int64_t>>>
			static I Generate_int(std::mt19937& engine, I min, I max) {
			std::uniform_int_distribution<I> dis(min, max);
			return dis(engine);
		}

		template <typename F>
		static F Generate_float(std::mt19937& engine) {
			std::uniform_real_distribution<F> dis(
				std::numeric_limits<F>::min(),
				std::numeric_limits<F>::max()
			);
			return dis(engine);
		}

		template <typename F>
		static F Generate_float(std::mt19937& engine, F min, F max) {
			std::uniform_real_distribution<F> dis(min, max);
			return dis(engine);
		}

	public:
		template <typename T>
		static T Get()
		{
			std::random_device rd;
			std::mt19937 gen(rd());
			return Generate_int<T>(gen);
		}

		template <typename T>
		static T Get(T min, T max)
		{
			std::random_device rd;
			std::mt19937 gen(rd());
			return Generate_int<T>(gen, min, max);
		}

		static float Get(float min, float max)
		{
			std::random_device rd;
			std::mt19937 gen(rd());
			return Generate_float<float>(gen, min, max);
		}

		static double Get(double min, double max)
		{
			std::random_device rd;
			std::mt19937 gen(rd());
			return Generate_float<double>(gen, min, max);
		}
	};
}

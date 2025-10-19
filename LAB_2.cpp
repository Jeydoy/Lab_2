// Lab 2
// Lysenko Yevhenii K-27
// Variant 10 (for_each)
// Defaul compiler of VS Community 2022

#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <numeric>
#include <execution>
#include <thread>
#include <cmath>
#include <iomanip>
#include <string>
#include <functional>

void FastOp(double& x) {
	x = x * 1.1488 + 1;
}

void SlowOp(double& x) {
	for (size_t i = 0; i < 60; i++)
	{
		x = std::cos(x) + std::sin(x);
	}
}

using HighResClock = std::chrono::high_resolution_clock;
using DurationMs = std::chrono::duration<double, std::milli>;

std::vector<double> GenerateRandomVector(size_t Size) {
	std::vector<double> Vec(Size);
	std::mt19937 Gen(std::random_device{}());
	std::uniform_real_distribution<> Dis(0.0, 10.0);
	std::generate (Vec.begin(), Vec.end(), [&]() {return Dis(Gen); });
	return Vec;
}

void CustomParralelForEach(std::vector<double>& Data, size_t K, const std::function<void(double&)>& Op) {
	if (K == 0) return;

	std::vector<std::jthread> Threads;
	size_t ChunkSize = Data.size() / K;
	size_t Reminder = Data.size() % K;

	auto ItStart = Data.begin();

	for (size_t i = 0; i < K; i++)
	{
		size_t CurrentChunkSize = ChunkSize + (i < Reminder ? 1 : 0);
		auto ItEnd = ItStart + CurrentChunkSize;

		Threads.emplace_back([ItStart, ItEnd, &Op]() {
			std::for_each(ItStart, ItEnd, Op); });
		ItStart = ItEnd;
	}
}

void RunExperiment(size_t DataSize, const std::function<void(double&)>& Op, const std::string& OpName) {
	std::cout << "-----------------------------------------------------------------------------\n";
	std::cout << "|| Operation Experiment: " << std::setw(10) << std::left << OpName << "               ||\n";
	std::cout << "|| Data size: " << std::setw(15) << std::left << DataSize << "                        ||\n";
	std::cout << "-----------------------------------------------------------------------------\n\n";

	auto OriginalData = GenerateRandomVector(DataSize);
	std::vector<double> Data;

	std::cout << "----1.  Library algorhytm experiments ----\n";
	std::cout << std::fixed << std::setprecision(5);

	Data = OriginalData;
	auto Start = HighResClock::now();
	std::for_each( Data.begin(), Data.end(), Op);
	auto End = HighResClock::now();
	DurationMs TimeSeqDefault = End - Start;
	std::cout << "Without policy(seq default): " << TimeSeqDefault.count() << "ms\n ";

	Data = OriginalData;
	Start = HighResClock::now();
	std::for_each(std::execution::seq, Data.begin(), Data.end(), Op);
	End = HighResClock::now();
	DurationMs TimeSeq = End - Start;
	std::cout << "Execution policy seq:     " << TimeSeq.count() << "ms\n";

	Data = OriginalData;
	Start = HighResClock::now();
	std::for_each(std::execution::par, Data.begin(), Data.end(), Op);
	End = HighResClock::now();
	DurationMs TimePar = End - Start;
	std::cout << "Execution policy par:    " << TimePar.count() << "ms\n";

	Data = OriginalData;
	Start = HighResClock::now();
	std::for_each(std::execution::par_unseq, Data.begin(), Data.end(), Op);
	End = HighResClock::now();
	DurationMs TimeParUnseq = End - Start;
	std::cout << "Execution policy par_unseq:     " << TimeParUnseq.count() << "ms\n";

	
	
	std::cout << "----2. Experiment within paralel algorhytm ---- \n";

	unsigned int HardwareThreads = std::thread::hardware_concurrency();
	std::cout << "Amount of aparat threads on this CPU: " << HardwareThreads << "\n\n";
	std::cout << "-------------------------------------\n";
	std::cout << "| K | Time of working (ms)       |\n";
	std::cout << "-------------------------------------\n";

	DurationMs BestTime = DurationMs::max();
	size_t BestK = 0;

	size_t MaxK = HardwareThreads * 2 + 2;
	if (MaxK > 32) MaxK = 32;

	for (size_t K = 1; K < MaxK; K++)
	{
		Data = OriginalData;
		Start = HighResClock::now();
		CustomParralelForEach(Data, K, Op);
		End = HighResClock::now();
		DurationMs CurrentTime = End - Start;

		std::cout << "| " << std::setw(6) << K << " | " << std::setw(12) << std::fixed << std::setprecision(4) << CurrentTime.count() << " | \n";

		if (K == 1) {
			BestTime = CurrentTime;
			BestK = K;
		}
		else if (CurrentTime < BestTime) {
			BestTime = CurrentTime;
			BestK = K;
		}
	}

	std::cout << " ----------------------------------\n\n";

	std::cout << "----3. Conclusions for that test ----\n";
	std::cout << "The best algorhytm speed is rached if K = " << BestK << "\n";
	std::cout << "Working time: " << BestTime.count() << "ms\n";

	double Ratio = static_cast<double>(BestK) / HardwareThreads;
	std::cout << "Comparance K to amount of hardware threads: " << BestK << "/" << HardwareThreads << "=" << std::fixed << Ratio << "\n";

	std::cout << "\n Observations: \n";
	std::cout << "1. For a fast operation, the overhead of creating and synchronizing threads may\n";
	std::cout << "exceed the gain from parallelization. Sometimes the serial version can be faster. \n";
	std::cout << "2. For slow operation advantages of parallelization become evidently. The acceleration is significant";
	std::cout << "3. As K increases beyond the number of hardware threads, execution time typicallyy increases due to the\n";
	std::cout << "overhead of context switching between threads.\n\n";
}

int main() {
	std::vector<size_t> DataSizes = { 1'000'000,10'000'000 };

	for (size_t Size : DataSizes) {
		RunExperiment(Size, FastOp, "FastOp");
		RunExperiment(Size, SlowOp, "SlowOp");
	}
	return 0;
}
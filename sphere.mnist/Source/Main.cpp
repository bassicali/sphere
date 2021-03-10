
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <string>
#include <iostream>
#include <filesystem>
#include <vector>

#include <windows.h>

#include "Trainer.h"
#include "Tester.h"
#include "Constants.h"
#include "Visualizer.h"

using namespace std;
using namespace sphere;

struct ProgramParams
{
	int NumHardLocations = NUM_HARD_LOC;
	int TrainingCount = TRAINING_SET_LIMIT;
	int RecallCount = 100;
	int StartFrom = 0;
	int LogDistances = 0;
	int SaveVisuals = 0;
	int AdjustWeights = 1;
	int SegmentImprints = 1;

#if MANHATTAN_DISTANCE
	float ImprintWeight = 0.70f;
#else
	float ImprintWeight = 0.28f;
#endif
	string InputImages1 = string("train-images.idx3-ubyte");
	string InputLabels1 = string("train-labels.idx1-ubyte");

	string InputImages2 = string("t10k-images.idx3-ubyte");
	string InputLabels2 = string("t10k-labels.idx1-ubyte");

	string MemFile = string("mnist.sph");
} params;

Trainer* trainer = nullptr;

float weights[10] =
{
	1.0f, // 0
	2.5f, // 1
	1.0f, // 2
	0.8f, // 3
	1.0f, // 4
	1.0f, // 5
	1.0f, // 6
	1.5f, // 7
	0.7f, // 8
	0.9f  // 9
};

typedef void (*Subroutine_Func)(void);
struct Subroutine
{
	string Name;
	Subroutine_Func Func;

	Subroutine(string name, Subroutine_Func func)
	{
		Name = name;
		Func = func;
	}
};


BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	if (fdwCtrlType == CTRL_C_EVENT && trainer)
	{
		LOG_INFO("** Ctrl-C **");

		if (trainer->IsTraining())
		{
			trainer->StopTraining();

			LOG_INFO("Waiting for Trainer to save work");
			while (trainer->IsTraining())
			{
				Sleep(1000);
			}
		}
	}

	return FALSE;
}

void TrainAndRecall()
{
	LOG_INFO("Training with MNIST data");

	trainer = new Trainer(params.InputImages1, params.InputLabels1, params.NumHardLocations);

	float* in_weights = params.AdjustWeights ? weights : nullptr;
	trainer->InitializeHardLocationsAddrs(params.ImprintWeight, in_weights, params.SegmentImprints);

	trainer->TrainMemory(nullptr, 0, params.TrainingCount, params.LogDistances, params.SaveVisuals);

	LOG_INFO("Recalling with learned data");
	Tester tester1(params.InputImages1, params.InputLabels1);
	auto results1 = tester1.TestImages(trainer->Memory(), params.RecallCount);
	results1.Print();

	LOG_INFO("Recalling with unlearned data");
	Tester tester2(params.InputImages2, params.InputLabels2);
	auto results2 = tester2.TestImages(trainer->Memory(), params.RecallCount);

	results2.Print();

	if (!params.MemFile.empty())
	{
		trainer->Memory().SaveToFile(params.MemFile);
	}
}

void TrainMemory()
{
	trainer = new Trainer(params.InputImages1, params.InputLabels1, params.NumHardLocations);

	float* in_weights = params.AdjustWeights ? weights : nullptr;
	trainer->InitializeHardLocationsAddrs(params.ImprintWeight, in_weights, false);

	LOG_INFO("Training with MNIST data");
	trainer->TrainMemory(params.MemFile.c_str(), 0, params.TrainingCount, params.LogDistances, params.SaveVisuals);
}

void Recall()
{
	LOG_INFO("Testing trained memory");

	LOG_INFO("Loading memory: %s", params.MemFile.c_str());
	auto sdm = Memory::LoadFromFile(params.MemFile);

	Tester tester(params.InputImages1, params.InputLabels1);
	auto results = tester.TestImages(sdm, params.TrainingCount);
	results.Print();
}

void TestSerialization()
{
	TrainMemory();

	LOG_INFO("Loading memory from file: %s", params.MemFile.c_str());
	Memory mem = Memory::LoadFromFile(params.MemFile);

	LOG_INFO("Saving memory back to file: mnist2.sph");
	mem.SaveToFile("mnist2.sph");

	// Compare the hash of the two files
}


int main(int argc, char** argv)
{
	namespace fs = std::filesystem;

#define PARSE_INT_ARG(str,prefix,output) if (str.rfind(prefix,0) == 0) { string num = str.substr(prefix.length()); params.output = atoi(num.c_str()); }
#define PARSE_FLT_ARG(str,prefix,output) if (str.rfind(prefix,0) == 0) { string num = str.substr(prefix.length()); params.output = atof(num.c_str()); }
#define PARSE_STR_ARG(str,prefix,output) if (str.rfind(prefix,0) == 0) { params.output = str.substr(prefix.length()); }

	vector<Subroutine> routines;
	routines.push_back(Subroutine("train", &TrainMemory));
	routines.push_back(Subroutine("recall", &Recall));
	routines.push_back(Subroutine("train_recall", &TrainAndRecall));
	routines.push_back(Subroutine("serialization-test", &TestSerialization));

	vector<string> args;
	for (int i = 0; i < argc; i++)
		args.push_back(string(argv[i]));

	Subroutine_Func routine = nullptr;
	for (auto& r : routines)
	{
		if (args[1].compare(r.Name) == 0)
		{
			routine = r.Func;
			break;
		}
	}

	if (!routine)
	{
		cout << "Invalid arguments." << endl;
		return 1;
	}

	for (int i = 2; i < args.size(); i++)
	{
		PARSE_INT_ARG(args[i], string("--count="), TrainingCount);
		PARSE_INT_ARG(args[i], string("--rcount="), RecallCount);
		PARSE_INT_ARG(args[i], string("--start="), StartFrom);
		PARSE_INT_ARG(args[i], string("--hl="), NumHardLocations);
		PARSE_FLT_ARG(args[i], string("--imprint="), ImprintWeight);
		PARSE_FLT_ARG(args[i], string("--segment-imprints="), SegmentImprints);
		PARSE_FLT_ARG(args[i], string("--adjust-weights="), AdjustWeights);
		PARSE_INT_ARG(args[i], string("--log-distances="), LogDistances);
		PARSE_INT_ARG(args[i], string("--save-visuals="), SaveVisuals);
		PARSE_STR_ARG(args[i], string("--images="), InputImages1);
		PARSE_STR_ARG(args[i], string("--labels="), InputLabels1);
		PARSE_STR_ARG(args[i], string("--file="), MemFile);
	}

	if (SetConsoleCtrlHandler(CtrlHandler, TRUE) == 0)
	{
		cout << "Failed to set console ctrl handler" << endl;
		return 1;
	}

	string filename("sphere-log.txt");

	if (fs::exists(filename))
	{
		time_t result = time(nullptr);
		char filename_buff[128];
		sprintf_s(filename_buff, "sphere-log-%d.txt", result);
		fs::rename(filename, filename_buff);
	}

	HANDLE log_file = CreateFile(filename.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (log_file != INVALID_HANDLE_VALUE)
	{
		SetLogFile(log_file);
	}
	else
	{
		LOG_ERROR("Could not create log file");
	}

	LOG_INFO("Parameters:");
	LOG_INFO("\tMode: %s", args[1].c_str());
	LOG_INFO("\tData set 1: %s + %s", params.InputImages1.c_str(), params.InputLabels1.c_str());
	LOG_INFO("\tData set 2: %s + %s", params.InputImages2.c_str(), params.InputLabels2.c_str());
	LOG_INFO("\tFile: %s", params.MemFile.c_str());
	LOG_INFO("\tAccess Sphere Radius: %d", RADIUS);
	LOG_INFO("\tHard locations: %d", params.NumHardLocations);
	LOG_INFO("\tImprint weight: %.3f", params.ImprintWeight);
	LOG_INFO("\tSegment imprints: %d", params.SegmentImprints);
	LOG_INFO("\tUse weight adjustments: %d", params.AdjustWeights);
	LOG_INFO("\tTraining count: %d", params.TrainingCount);
	LOG_INFO("\tRecall count: %d", params.RecallCount);
	LOG_INFO("\tStart from: %d", params.StartFrom);
	LOG_INFO("\tImage distances to log: %d", params.LogDistances);
	LOG_INFO("\tVisual bitmaps to save: %d", params.SaveVisuals);

	try
	{
		routine();
	}
	catch (exception& ex)
	{
		cout << "Unhandled error: " << ex.what() << endl;
		return 1;
	}
	catch (...)
	{
		cout << "Unhandled error" << endl;
		return 1;
	}

	return 0;
}
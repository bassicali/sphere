
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <string>
#include <iostream>
#include <vector>

#include <windows.h>

#include "Trainer.h"
#include "Tester.h"
#include "Params.h"
#include "Visualizer.h"

using namespace std;
using namespace sphere;

int mode = -1;
int num_hard_locations = NUM_HARD_LOC;
int training_count = TRAINING_SET_LIMIT;
int log_distances = 0;
int save_visuals = 0;
string output_filename("mnist.sph");

Trainer* trainer;

enum class SubRoutine : int
{
	Recall,
	Train,
	TestSerialization
};

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	if (fdwCtrlType == CTRL_C_EVENT && mode == (int)SubRoutine::Train && trainer)
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

		return TRUE;
	}

	return FALSE;
}

void TrainingSubRoutine()
{
	LOG_INFO("Training with MNIST data");

	trainer = new Trainer("train-images.idx3-ubyte", "train-labels.idx1-ubyte", num_hard_locations);
	trainer->TrainMemory(output_filename.c_str(), training_count, log_distances, save_visuals);
}

void RecallSubRoutine()
{
	LOG_INFO("Testing trained memory");

	Tester tester("t10k-images.idx3-ubyte", "t10k-labels.idx1-ubyte", output_filename);
	tester.TestImages(100);
}

void TestSerialization()
{
	TrainingSubRoutine();

	LOG_INFO("Loading memory from file: %s", output_filename.c_str());
	Memory mem = Memory::LoadFromFile(output_filename);

	LOG_INFO("Saving memory back to file: mnist2.sph");
	mem.SaveToFile("mnist2.sph");

	// Compare the hash of the two files
}

int main(int argc, char** argv)
{
#define PARSE_INT_ARG(str,prefix,output) if (str.rfind(prefix,0) == 0) { string num = str.substr(prefix.length()); output = atoi(num.c_str()); }
#define PARSE_STR_ARG(str,prefix,output) if (str.rfind(prefix,0) == 0) { output = str.substr(prefix.length()); }

	vector<string> args;
	for (int i = 0; i < argc; i++)
		args.push_back(string(argv[i]));

	if (args[1].compare("train") == 0)
		mode = (int)SubRoutine::Train;
	else if (args[1].compare("recall") == 0)
		mode = (int)SubRoutine::Recall;
	else if (args[1].compare("serialization-test") == 0)
		mode = (int)SubRoutine::TestSerialization;

	if (mode == -1)
	{
		cout << "Invalid arguments." << endl;
		return 1;
	}

	for (int i = 2; i < args.size(); i++)
	{
		PARSE_INT_ARG(args[i], string("--count="), training_count);
		PARSE_INT_ARG(args[i], string("--hl="), num_hard_locations);
		PARSE_INT_ARG(args[i], string("--log-distances="), log_distances);
		PARSE_INT_ARG(args[i], string("--save-visuals="), save_visuals);
		PARSE_STR_ARG(args[i], string("--output="), output_filename);
	}

	if (SetConsoleCtrlHandler(CtrlHandler, TRUE) == 0)
	{
		cout << "Failed to set console ctrl handler" << endl;
		return 1;
	}

	CreateLogFile("sphere-log.txt");

	LOG_INFO("Num hard locations: %d", num_hard_locations);
	LOG_INFO("Training count: %d", training_count);
	LOG_INFO("Output file: %s", output_filename.c_str());
	LOG_INFO("Num image distances to log: %d", log_distances);
	LOG_INFO("Num visual bitmaps to save: %d", save_visuals);

	try
	{
		switch (SubRoutine(mode))
		{
			case SubRoutine::Train:
				TrainingSubRoutine();
				break;
			case SubRoutine::Recall:
				RecallSubRoutine();
				break;
			case SubRoutine::TestSerialization:
				TestSerialization();
				break;
		}
	}
	catch (exception& ex)
	{
		cout << "Unhandled error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}
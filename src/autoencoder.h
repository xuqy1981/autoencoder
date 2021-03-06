#ifndef AUTOENCODER
#define AUTOENCODER

#include "auto_hidden_layer.h"
#include "neural_network.h"
#include "ga.h"
#include <random>

using namespace std;

class autoencoder: public neural_network
{
private:
  int numPreTrainLayers;
  vector<float> preTrainLayersLearnRates;
  vector<int> preTrainLayersOuterIter;
  vector<float> preTrainLayersNoiseLevels;

  vector<auto_hidden_layer *> preTrainLayers;
  vector<float *> preTrainLayersOutputs;
  hidden_layer *output_l;
  ga_params myParams;

public:

  autoencoder(vector<int> preTrainLayerWidths, vector<float> preTrainLayersLearnRates, vector<int> preTrainLayersOuterIter, vector<float> preTrainLayersNoiseLevels, int numInput, int numHidden, int numOutput, float learn_rate);
  ~autoencoder();

  void getInputK(float *&o_i, int k);
  void getInput(float *&o_i);
  void corrupt_masking(float *input, float *corrupted_input, float fraction, int length);
  void corrupt_gaussian(float *input, float *corrupted_input, float sigma, int length);

  void setO_i(float **o_i, float **trainingImages, int k, int numTrainingImages, int length);
  void setCorruptedO_i(float **corrupted_o_i, float **o_i, int k, int numTrainingImages, int length);
  void deleteO_i(float **o_i, int numTrainingImages);

  void preTrain(float **trainingImages, int numTrainingImages);
  void preTrainGA(float **trainingImages, int numTrainingImages);
  void preTrainGAMiniBatch(float **trainingImages, int numTrainingImages);
  // This function train doesn't seem to exist in the autoencoder class
  void train(float **trainingImages, vector<int> &trainLabels, int numOuterIter, int numTrainingImages);

  void fineTune(float **trainingImages, int numTrainingImages, vector<int> &trainLabels);
  void fineTuneNoHidden(float **trainingImages, int numTrainingImages, vector<int> &trainLabels);
  void testFineNoHidden(float **testingImages, vector<int> &testLabels, int numTestingImages);
  int predictFineNoHidden(float *o_i);

  void reconstructImage(float **testingImages, int layer, int n);
  void visualizeWeights(int layer, int n);

  inline ga_params *getMyParams()
  {
    return &myParams;
  }
};

#endif

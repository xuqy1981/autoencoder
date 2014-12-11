#include "autoencoder.h"

inline float uniformRandom()
{
  return ( (float)(rand()) + 1. ) / ( (float)(RAND_MAX) + 1. );
}

inline float normalRandom()
{
  float u1 = uniformRandom();
  float u2 = uniformRandom();
  return cos(8.*atan(1.) * u2) * sqrt(-2.*log(u1));
}

autoencoder::autoencoder(vector<int> preTrainLayerWidths, vector<float> preTrainLayersLearnRates, vector<int> preTrainLayersOuterIter, vector<float> preTrainLayersNoiseLevels, int numInput, int numHidden, int numOutput, float learn_rate):
  neural_network(numInput, numHidden, numOutput, learn_rate),
  preTrainLayersLearnRates(preTrainLayersLearnRates),
  preTrainLayersOuterIter(preTrainLayersOuterIter),
  preTrainLayersNoiseLevels(preTrainLayersNoiseLevels)

{
  numPreTrainLayers = (int) preTrainLayerWidths.size();
  cout << "created an stacked denoising autoencoder with " << numPreTrainLayers << " pretrained layers" << endl;
  for (int i = 0; i < numPreTrainLayers; i++)
  {
    int auto_num_input = preTrainLayerWidths[i];
    int auto_num_output;
    if (i == numPreTrainLayers - 1)
    {
      auto_num_output = numInput;
    }
    else
    {
      auto_num_output = preTrainLayerWidths[i + 1];
    }

    auto_hidden_layer *x = new auto_hidden_layer(auto_num_input, auto_num_output);
    preTrainLayers.push_back(x);
    float *output = new float[auto_num_output];
    preTrainLayersOutputs.push_back(output);

    cout << "layer " << i + 1 << ": number of outer iterations " << preTrainLayersOuterIter[i] << ", learning rate " << preTrainLayersLearnRates[i] << ", number of hidden units " << auto_num_output << ", noise level " << preTrainLayersNoiseLevels[i] << endl;
  }

}

void autoencoder::corrupt_masking(float *input, float *corrupted_input, float fraction, int length)
{
  // #pragma omp parallel for schedule(static, 2)
  for (int i = 0; i < length; i++)
  {
    if (uniformRandom() < fraction)
    {
      corrupted_input[i] = 0.0;
    }
    else
    {
      corrupted_input[i] = input[i];
    }
  }
}

void autoencoder::corrupt_gaussian(float *input, float *corrupted_input, float sigma, int length)
{
  // #pragma omp parallel for schedule(static, 50)
  for (int i = 0; i < length; i++)
  {
    corrupted_input[i] = input[i] + normalRandom() * sigma;
  }
}

void autoencoder::getInputK(float *&o_i, int k)
{
  for (int i = 0; i < k; i++)
  {
    preTrainLayers[i]->encode(o_i, preTrainLayersOutputs[i]);
    o_i = preTrainLayersOutputs[i];
  }
}

void autoencoder::getInput(float *&o_i)
{
  for (int i = 0; i < numPreTrainLayers; i++)
  {
    preTrainLayers[i]->encode(o_i, preTrainLayersOutputs[i]);
    o_i = preTrainLayersOutputs[i];
  }
}

void autoencoder::setO_i(float **o_i, float **trainingImages, int k, int numTrainingImages, int length)
{
  for (int i = 0; i < numTrainingImages; i++)
  {
    o_i[i] = new float[length];
  }

  for (int i = 0; i < numTrainingImages; i++)
  {
    float *o_t = trainingImages[i];
    getInputK(o_t, k);
    for (int j = 0; j < length; j++)
    {
      o_i[i][j] = o_t[j];
    }
  }
}

void autoencoder::setCorruptedO_i(float **corrupted_o_i, float **o_i, int k, int numTrainingImages, int length)
{
  for (int i = 0; i < numTrainingImages; i++)
  {
    corrupted_o_i[i] = new float[length];
    corrupt_masking(o_i[i], corrupted_o_i[i], preTrainLayersNoiseLevels[k], length);
  }
}

void autoencoder::deleteO_i(float **o_i, int numTrainingImages)
{
  for (int i = 0; i < numTrainingImages; i++)
  {
    delete[] o_i[i];
  }
  delete[] o_i;
}

void autoencoder::preTrain(float **trainingImages, int numTrainingImages)
{
  for (int k = 0; k < numPreTrainLayers; k++)
  {

    cout << "pretraining layer #" << k + 1 << ", cycling through " << numTrainingImages << " training images for " << preTrainLayersOuterIter[k] << " outer iterations" << endl;

    auto_hidden_layer *a = preTrainLayers[k];

    float **o_i = new float*[numTrainingImages];
    float **corrupted_o_i = new float*[numTrainingImages];
    if (k > 0)
    {
      setO_i(o_i, trainingImages, k, numTrainingImages, a->getNumInputUnits());
    }
    else
    {
      o_i = trainingImages;
    }
    setCorruptedO_i(corrupted_o_i, o_i, k, numTrainingImages, a->getNumHiddenUnits());

    float *o_e = new float[a->getNumHiddenUnits()];
    float *delta_e = new float[a->getNumHiddenUnits()];
    float *o_d = new float[a->getNumInputUnits()];
    float *delta_d = new float[a->getNumInputUnits()];

    float sum_squared_error = 0.0;
    for (int i = 0; i < numTrainingImages; i++)
    {
      float *o_t = o_i[i];
      getInputK(o_t, k);

      a->encode(o_t, o_e);
      a->decode(o_e, o_d);

      sum_squared_error += a->auto_squared_loss(o_t, o_d);
    }
    cout << "outer iter: 0 wall time: 0.00000 total error: " << sum_squared_error << endl;

    double start = omp_get_wtime();
    for (int i = 0; i < preTrainLayersOuterIter[k]; i++ )
    {
      // float total_encode_time = 0;
      // float total_decode_time = 0;
      // float total_compute_delta_output_time = 0;
      // float total_compute_delta_hidden_time = 0;
      // float total_updateWeights_time = 0;
      // float total_squared_loss_time = 0;
      // float t;

      float sum_squared_error = 0.0;
      for (int j = 0; j < numTrainingImages; j++)
      {
        float *o_t = o_i[j];
        float *corrupted_o_t = corrupted_o_i[j];

        // t = omp_get_wtime();
        a->encode(corrupted_o_t, o_e);
        // total_encode_time += omp_get_wtime() - t;

        // t = omp_get_wtime();
        a->decode(o_e, o_d);
        // total_decode_time += omp_get_wtime() - t;

        // t = omp_get_wtime();
        a->auto_compute_delta_output(delta_d, o_d, o_t);
        // total_compute_delta_output_time += omp_get_wtime() - t;

        // t = omp_get_wtime();
        a->auto_compute_delta_hidden(delta_e, delta_d, o_e);
        // total_compute_delta_hidden_time += omp_get_wtime() - t;

        // t = omp_get_wtime();
        a->auto_updateWeights(delta_e, corrupted_o_t, delta_d, o_e, preTrainLayersLearnRates[k]);
        // total_updateWeights_time += omp_get_wtime() - t;

        // t = omp_get_wtime();
        sum_squared_error += a->auto_squared_loss(o_t, o_d);
        // total_squared_loss_time += omp_get_wtime() - t;

      }
      cout << "outer iter: " << i + 1 << " wall time: " << omp_get_wtime() - start << " total error: " << sum_squared_error << endl;
      // cout << "total_encode_time: " << total_encode_time << endl;
      // cout << "total_decode_time: " << total_decode_time << endl;
      // cout << "total_compute_delta_output_time: " << total_compute_delta_output_time << endl;
      // cout << "total_compute_delta_hidden_time: " << total_compute_delta_hidden_time << endl;
      // cout << "total_updateWeights_time: " << total_updateWeights_time << endl;
      // cout << "total_squared_loss_time: " << total_squared_loss_time << endl;
      // cout << endl;
    }

    if (k > 0)
    {
      deleteO_i(o_i, numTrainingImages);
    }
    deleteO_i(corrupted_o_i, numTrainingImages);

    delete[] o_e;
    delete[] o_d;
    delete[] delta_e;
    delete[] delta_d;
  }
}

void autoencoder::preTrainGA(float **trainingImages, int numTrainingImages)
{
  for (int k = 0; k < numPreTrainLayers; k++)
  {

    cout << "pretraining layer #" << k + 1 << "with GA, cycling through " << numTrainingImages << " training images for " << preTrainLayersOuterIter[k] << " outer iterations" << endl;

    auto_hidden_layer *a = preTrainLayers[k];

    float **o_i = new float*[numTrainingImages];
    float **corrupted_o_i = new float*[numTrainingImages];
    if (k > 0)
    {
      setO_i(o_i, trainingImages, k, numTrainingImages, a->getNumInputUnits());
    }
    else
    {
      o_i = trainingImages;
    }
    setCorruptedO_i(corrupted_o_i, o_i, k, numTrainingImages, a->getNumHiddenUnits());

    float *o_e = new float[a->getNumHiddenUnits()];
    float *o_d = new float[a->getNumInputUnits()];

    float sum_squared_error = 0.0;
    for (int i = 0; i < numTrainingImages; i++)
    {
      float *o_t = o_i[i];
      getInputK(o_t, k);
      a->encode(o_t, o_e);
      a->decode(o_e, o_d);
      sum_squared_error += a->auto_squared_loss(o_t, o_d);
    }
    cout << "outer iter: 0 wall time: 0.00000 total error: " << sum_squared_error << endl;

    double start = omp_get_wtime();

    //setup GA and its parameters
    ga_params myParams;
    myParams.mutRate = 0.1;
    myParams.mutAmount = 0.1 * a->getWeightRange();
    myParams.crossRate = 0.5;
    myParams.popSize = 50;
    myParams.genomeSize = a->getNumWeights() + a->getNumInputUnits() + a->getNumHiddenUnits();
    myParams.numToReplace = 25;
    myParams.initRange = 1.0 * a->getWeightRange();
    genetic *ga = new genetic(myParams);

    delete[] a->getWeights();
    delete[] a->getEncodeBiases();
    delete[] a->getDecodeBiases();
    int encodeBiasesOffset = a->getNumWeights();
    int decodeBiasesOffset = encodeBiasesOffset + a->getNumHiddenUnits();

    for (int i = 0; i < preTrainLayersOuterIter[k]; i++ )
    {

      float sum_squared_error = 0.0;
      for (int j = 0; j < numTrainingImages; j++)
      {
        float *o_t = o_i[j];
        float *corrupted_o_t = corrupted_o_i[j];
        ga->step();

        for (int currInd = 0; currInd < myParams.numToReplace; currInd++)
        {
          float *genome = ga->getGenome(currInd);
          a->setWeights(genome);
          a->setEncodeBiases(&(genome[encodeBiasesOffset]));
          a->setDecodeBiases(&(genome[decodeBiasesOffset]));
          a->encode(corrupted_o_t, o_e);
          a->decode(o_e, o_d);
          float error = a->auto_squared_loss(o_t, o_d);
          ga->setFitness(currInd, 1.0 / log(error * numTrainingImages));
        }

        float *genome = ga->getGenome(myParams.popSize - 1);
        a->setWeights(genome);
        a->setEncodeBiases(&(genome[encodeBiasesOffset]));
        a->setDecodeBiases(&(genome[decodeBiasesOffset]));
        a->encode(corrupted_o_t, o_e);
        a->decode(o_e, o_d);
        sum_squared_error += a->auto_squared_loss(o_t, o_d);
      }
      cout << "outer iter: " << i + 1 << " wall time: " << omp_get_wtime() - start << " total error: " << sum_squared_error << endl;
    }

    //cleanup
    if (k > 0)
    {
      deleteO_i(o_i, numTrainingImages);
    }
    deleteO_i(corrupted_o_i, numTrainingImages);

    delete[] o_e;
    delete[] o_d;

    delete ga;
  }
}

void autoencoder::train(float **trainingImages, vector<int> &trainLabels, int numOuterIter, int numTrainingImages)
{
  cout << "cycling through " << numTrainingImages << " training images for " << numOuterIter << " outer iterations" << endl;

  float **o_i = new float*[numTrainingImages];
  setO_i(o_i, trainingImages, numPreTrainLayers, numTrainingImages, h->getNumInputUnits());

  float sum_squared_error = 0.0;
  for (int j = 0; j < numTrainingImages; j++)
  {
    float *_o_i = o_i[j];

    h->encode(_o_i, o_j);
    o->encode(o_j, o_k);

    sum_squared_error += o->squared_loss(o_k, trainLabels[j]);
  }
  cout << "outer iter: 0 wall time: 0.00000 total error: " << sum_squared_error << endl;

  double start = omp_get_wtime();
  for (int i = 0; i < numOuterIter; i++ )
  {
    float sum_squared_error = 0.0;
    for (int j = 0; j < numTrainingImages; j++)
    {
      float *_o_i = o_i[j];

      sum_squared_error += backprop(_o_i, trainLabels[j]);
    }
    cout << "outer iter: " << i + 1 << " wall time: " << omp_get_wtime() - start << " total error: " << sum_squared_error << endl;
  }

  deleteO_i(o_i, numTrainingImages);
}

void autoencoder::visualizeWeights(int layer, int n)
{
  cout << "printing weights for first " << n << " hidden units of layer " << layer + 1 << endl;
  auto_hidden_layer *myLayer = preTrainLayers[layer];
  myLayer->printWeights(n);
}

void autoencoder::reconstructImage(float **testingImages, int layer, int n)
{
  cout << "printing " << n << " reconstructed test images for " << layer + 1 << endl;
  auto_hidden_layer *myLayer = preTrainLayers[layer];
  float *myLayerOutput = preTrainLayersOutputs[layer];
  float *reconstructed_output = new float[784];
  float *corrupted_input = new float[784];
  for (int i = 0; i < n; i++)
  {
    float *image = testingImages[i];
    corrupt_masking(image, corrupted_input, 0.25, 784);
    myLayer->encode(corrupted_input, myLayerOutput);
    myLayer->decode(myLayerOutput, reconstructed_output);
    for (int j = 0; j < 784; j++)
    {
      cout << corrupted_input[j] << " ";
    }
    cout << endl;
    for (int j = 0; j < 784; j++)
    {
      cout << reconstructed_output[j] << " ";
    }
    cout << endl;
  }
  delete[] reconstructed_output;
  delete[] corrupted_input;
}

autoencoder::~autoencoder()
{
  while (!preTrainLayers.empty())
  {
    delete preTrainLayers.back();
    preTrainLayers.pop_back();
  }
  while (!preTrainLayersOutputs.empty())
  {
    delete[] preTrainLayersOutputs.back();
    preTrainLayersOutputs.pop_back();
  }
}
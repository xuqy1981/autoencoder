#include "autoencoder.h"

autoencoder::autoencoder(vector<int> preTrainLayerWidths, int numInput, int numHidden, int numOutput, float learn_rate):
  neural_network(numInput, numHidden, numOutput, learn_rate)
{
	numPreTrainLayers = (int) preTrainLayerWidths.size();
  cout << "created an autoencoder with " << numPreTrainLayers << " pretrained layers, learn rate " << learn_rate << endl;

  for (int i = 0; i < numPreTrainLayers; i++)
  {
  	int auto_num_input = preTrainLayerWidths[i];
  	int auto_num_output;
  	if (i == numPreTrainLayers - 1) {
  		auto_num_output = numInput;
  	} else {
  		auto_num_output = preTrainLayerWidths[i+1];
  	}

    auto_hidden_layer *x = new auto_hidden_layer(auto_num_input, auto_num_output);
    preTrainLayers.push_back(x);
    float *output = new float[auto_num_output];
    preTrainLayersOutputs.push_back(output);
  }

}

void autoencoder::preTrain(float **trainingImages, int numTrainingImages, int numOuterIter)
{
  auto_hidden_layer *a = preTrainLayers[0];

  float *o_e = new float[a->getNumHiddenUnits()];
  float *delta_e = new float[a->getNumHiddenUnits()];

  float *o_d = new float[a->getNumInputUnits()];
  float *delta_d = new float[a->getNumInputUnits()];

  cout << "cycling through " << numTrainingImages << " training images for " << numOuterIter << " outer iterations" << endl;
  double start = omp_get_wtime();
  for (int i = 0; i < numOuterIter; i++ )
  {
    float total_encode_time = 0;
    float total_decode_time = 0;
    float total_compute_delta_output_time = 0;
    float total_compute_delta_hidden_time = 0;
    float total_updateWeights_time = 0;
    float total_squared_loss_time = 0;
    float t;

    float sum_squared_error = 0.0;
    for (int j = 0; j < numTrainingImages; j++)
    {
      float *o_i = trainingImages[j];

      t = omp_get_wtime();
      a->encode(o_i, o_e);
      total_encode_time += omp_get_wtime() - t;

      t = omp_get_wtime();
      a->decode(o_e, o_d);
      total_decode_time += omp_get_wtime() - t;

      t = omp_get_wtime();
      a->compute_delta_output(delta_d, o_d, o_i);
      total_compute_delta_output_time += omp_get_wtime() - t;

      t = omp_get_wtime();
      a->compute_delta_hidden(delta_e, delta_d, o_e);
      total_compute_delta_hidden_time += omp_get_wtime() - t;

      t = omp_get_wtime();
      a->updateWeights(delta_e, o_i, delta_d, o_e, learn_rate);
			total_updateWeights_time += omp_get_wtime() - t;

			t = omp_get_wtime();
      sum_squared_error += a->squared_loss(o_i, o_d);
      total_squared_loss_time += omp_get_wtime() - t;

    }
    cout << "outer iter: " << i + 1 << " wall time: " << omp_get_wtime() - start << " total error: " << sum_squared_error << endl;
    cout << "total_encode_time: " << total_encode_time << endl;
    cout << "total_decode_time: " << total_decode_time << endl;
    cout << "total_compute_delta_output_time: " << total_compute_delta_output_time << endl;
    cout << "total_compute_delta_hidden_time: " << total_compute_delta_hidden_time << endl;
    cout << "total_updateWeights_time: " << total_updateWeights_time << endl;
    cout << "total_squared_loss_time: " << total_squared_loss_time << endl;

    cout << endl;
  }

  delete[] o_e;
  delete[] o_d;
  delete[] delta_e;
  delete[] delta_d;
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
    delete preTrainLayersOutputs.back();
    preTrainLayersOutputs.pop_back();
  }
}
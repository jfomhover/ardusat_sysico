#ifndef ArduSatSensorHandler_H
#define ArduSatSensorHandler_H

template <typename T> class ArduSatSensorHandler
{
public:
  ArduSatSensorHandler(T * valueBuffer, int length);

  void reset();
  void pushValue(T value);
  T getValue();
  T getMinimum();
  T getMaximum();
  float getVariance();
  float getAverage();
  int getCount();

private:
  int bufferLength;
  int index;
  int count;
  int getCurrentMaxIndex();
  T * values;
};


template <typename T> ArduSatSensorHandler<T>::ArduSatSensorHandler(T * valueBuffer, int length) {
  bufferLength = length;
  values = valueBuffer;
};

template <typename T> void ArduSatSensorHandler<T>::reset() {
  for (int i=0; i<bufferLength; i++)
    values[i] = 0;
  index = 0;
  count = 0;
};

template <typename T> void ArduSatSensorHandler<T>::pushValue(T value) {
  values[index]=value;
  count++;
  index++;
  if (index >= bufferLength)
    index = 0;
};

template <typename T> int ArduSatSensorHandler<T>::getCount() {
  return(count);
};

template <typename T> int ArduSatSensorHandler<T>::getCurrentMaxIndex() {
  int t_index = index;
  if (count > index)
    t_index = bufferLength;
  return(t_index);
};

template <typename T> T ArduSatSensorHandler<T>::getValue() {
  if (index == 0) {
    if (count > 0)
      return(values[bufferLength-1]);
    else return(0);
  }
  return(values[index-1]);
};

template <typename T> T ArduSatSensorHandler<T>::getMinimum() {
  int t_index = getCurrentMaxIndex();
  
  int t_min = values[0];
  for (int i=1; i<t_index; i++) {
    if (values[i] < t_min)
      t_min = values[i];
  }
  return(t_min);
};

template <typename T> T ArduSatSensorHandler<T>::getMaximum() {
  int t_index = getCurrentMaxIndex();
  
  int t_max = values[0];
  for (int i=1; i<t_index; i++) {
    if (values[i] > t_max)
      t_max = values[i];
  }
  return(t_max);
};

template <typename T> float ArduSatSensorHandler<T>::getVariance() {
  int t_index = getCurrentMaxIndex();
  float t_avg = getAverage();
  float t_var = 0;
  
  for (int i=0; i<t_index; i++) {
    t_var += (values[i]-t_avg)*(values[i]-t_avg);
  }
  return(t_var/t_index);
};

template <typename T> float ArduSatSensorHandler<T>::getAverage() {
  int t_index = getCurrentMaxIndex();
  float t_avg = 0;

  for (int i=0; i<t_index; i++) {
    t_avg += values[i];
  }
  return(t_avg/t_index);
};


#endif

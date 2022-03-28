#ifndef OPTIM_H
#define OPTIM_H

/**
   \brief One iteration of a simple gradient search.

   \param eps Stepsize scaling factor.
   \retval param Input: start values, Output: new values.
   \param err Error function handle.
   \param data Data pointer to be handed to error function.
   \param unitstep Unit step size for numerical gradient calculation.
   \return Error measure.
 */
template <class T>
T downhill_iterate(T eps, std::vector<T>& param,
                   T (*err)(const std::vector<T>& x, void* data), void* data,
                   const std::vector<T>& unitstep)
{
  std::vector<T> stepparam(param);
  T errv(err(param, data));
  for(size_t k = 0; k < param.size(); k++) {
    stepparam[k] += unitstep[k];
    T dv(eps * (errv - err(stepparam, data)));
    stepparam[k] = param[k];
    param[k] += dv;
  }
  return errv;
}

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

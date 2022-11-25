#ifndef OPTIM_H
#define OPTIM_H

#include <stdlib.h>
#include <vector>

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

namespace TASCAR {

  /***
   * @brief minimizes a function using the Nelder-Mead algorithm.
   *
   * This routine seeks the minimum value of a user-specified function.
   *
   * Simplex function minimisation procedure due to Nelder+Mead(1965),
   * as implemented by O'Neill(1971, Appl.Statist. 20, 338-45), with
   * subsequent comments by Chambers+Ertel(1974, 23, 250-1),
   * Benyon(1976, 25, 97) and Hill(1978, 27, 380-2)
   *
   * The function to be minimized must have the form:
   *
   *    float fn( const std::vector<float>& X, void* data )
   *
   * where X is a vector, and which returns the (scalar) function
   * value.  This function must be passed as the argument FN.
   *
   * This routine does not include a termination test using the
   * fitting of a quadratic surface.
   *
   * @param xmin Return the coordinates of the point which is
   *    estimated to minimize the function.
   *
   * @param fn the function to be minimized
   *
   * @param start starting point for the iteration
   *
   * @param reqmin terminating limit for the variance of function
   *    values.
   *
   * @param step determines the size and shape of the initial simplex.
   *    The relative magnitudes of its elements should reflect the
   *    units of the variables.
   *
   * @param konvge the convergence check is carried out every konvge
   *    iterations.
   *
   * @param kcount the maximum number of function evaluations.
   *
   * @param data data pointer handed to the function to be minimized.
   *
   * @return error indicator.
   *    0, no errors detected.
   *    1, REQMIN, N, or KONVGE has an illegal value.
   *    2, iteration terminated because KCOUNT was exceeded without convergence.
   */
  int nelmin(std::vector<float>& xmin,
             float (*fn)(const std::vector<float>&, void* data),
             std::vector<float> start, float reqmin,
             const std::vector<float>& step, size_t konvge, size_t kcount,
             void* data);

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

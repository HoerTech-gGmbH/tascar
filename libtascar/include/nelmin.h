#include <vector>

namespace TASCAR {

  /***
   * @brief minimizes a function using the Nelder-Mead algorithm.
   *
   * This routine seeks the minimum value of a user-specified function.
   *
   *    Simplex function minimisation procedure due to Nelder+Mead(1965),
   *    as implemented by O'Neill(1971, Appl.Statist. 20, 338-45), with
   *    subsequent comments by Chambers+Ertel(1974, 23, 250-1), Benyon(1976,
   *    25, 97) and Hill(1978, 27, 380-2)
   *
   *    The function to be minimized must have the form:
   *
   *      function value = fn ( x )
   *
   *    where X is a vector, and VALUE is the (scalar) function value.
   *    The name of this function must be passed as the argument FN.
   *
   *    This routine does not include a termination test using the
   *    fitting of a quadratic surface.
   *
   *    \param fn the function to be minimized
   * \param start starting
   *    point for the iteration
   *
   *    real REQMIN, the terminating limit for the variance
   *    of function values.
   *
   *    real STEP(N), determines the size and shape of the
   *    initial simplex.  The relative magnitudes of its elements should reflect
   *    the units of the variables.
   *
   *    integer KONVGE, the convergence check is carried out
   *    every KONVGE iterations.
   *
   *    integer KCOUNT, the maximum number of function evaluations.
   *
   *  Output:
   *
   *    real XMIN(N), the coordinates of the point which
   *    is estimated to minimize the function.
   *
   *    real YNEWLO, the minimum value of the function.
   *
   *    integer ICOUNT, the number of function evaluations.
   *
   *    integer NUMRES, the number of restarts.
   *
   *    integer IFAULT, error indicator.
   *    0, no errors detected.
   *    1, REQMIN, N, or KONVGE has an illegal value.
   *    2, iteration terminated because KCOUNT was exceeded without convergence.
   */
  int nelmin(std::vector<float>& xmin, float (*fn)(const std::vector<float>&),
             std::vector<float> start, float reqmin,
             const std::vector<float>& step, size_t konvge, size_t kcount);

} // namespace TASCAR

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

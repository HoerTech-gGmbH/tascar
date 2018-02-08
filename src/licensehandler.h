#ifndef LICENSEHANDLER_H
#define LICENSEHANDLER_H

#include <string>
#include <map>
#include <set>

class licensehandler_t {
public:
  void add_license( const std::string& license, const std::string& attribution, const std::string& tag );
  std::string legal_stuff() const;
private:
  std::map<std::string,std::set<std::string> > licenses;
  std::map<std::string,std::set<std::string> > attributions;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * compile-command: "make -C .."
 * End:
 */

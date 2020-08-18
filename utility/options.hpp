#ifndef JHMI_UTILITY_OPTIONS_HPP_NRC_20171202
#define JHMI_UTILITY_OPTIONS_HPP_NRC_20171202

#include "utility/meta.hpp"
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include <boost/units/quantity.hpp>
#include <meta/meta.hpp>
#include <range/v3/all.hpp>
#include <iostream>
#include <random>

namespace jhmi {
  struct output_path_option {
    std::string var;
    void init(boost::program_options::options_description& desc) {
      desc.add_options()("output-path,o",
        boost::program_options::value<std::string>(&var)->required(),
        "path where the output directory will be created");
    }
    void parse(boost::program_options::variables_map const&) {}
    auto output_path() const { return boost::filesystem::path{var}; }
  };
  struct spherefile_option {
    std::string var;
    void init(boost::program_options::options_description& desc) {
      desc.add_options()("spherefile,p",
        boost::program_options::value<std::string>(&var)->required(),
        "pb file with sphere locations");
    }
    void parse(boost::program_options::variables_map const&) {}
    auto spherefile() const { return boost::filesystem::path{var}; }
  };
  struct spherefiles_option {
    std::vector<boost::filesystem::path> var;
    void init(boost::program_options::options_description& desc) {
      desc.add_options()("spherefiles,p",
        boost::program_options::value<std::vector<std::string>>()->required()->multitoken(),
        "list of spherefiles (sphere_locs.1.pbz).");
    }
    void parse(boost::program_options::variables_map const& vm) {
      ranges::transform(vm["spherefiles"].as<std::vector<std::string>>(),
                        ranges::back_inserter(var), [](std::string const& p) {
        return boost::filesystem::path{p};
      });
    }
    auto const& spherefiles() const { return var; }
  };
  struct shapefile_option {
    std::string var;
    void init(boost::program_options::options_description& desc) {
      desc.add_options()("shapefile,s",
        boost::program_options::value<std::string>(&var)->required(),
        "file with std::uints contained in a grid-based shape");
    }
    void parse(boost::program_options::variables_map const&) {}
    auto shapefile() const { return boost::filesystem::path{var}; }
  };
  struct treefile_option {
    std::string var;
    void init(boost::program_options::options_description& desc) {
      desc.add_options()("treefile,t",
        boost::program_options::value<std::string>(&var)->required(),
        "pb file with the hepatic artery tree and macrocells");
    }
    void parse(boost::program_options::variables_map const&) {}
    auto treefile() const { return boost::filesystem::path{var}; }
  };
  struct treefiles_option {
    std::vector<boost::filesystem::path> var;
    void init(boost::program_options::options_description& desc) {
      desc.add_options()("treefiles,t",
        boost::program_options::value<std::vector<std::string>>()->required()->multitoken(),
        "pb files with hepatic artery trees and macrocells");
    }
    void parse(boost::program_options::variables_map const& vm) {
      ranges::transform(vm["treefiles"].as<std::vector<std::string>>(),
                        ranges::back_inserter(var), [](std::string const& p) {
        return boost::filesystem::path{p};
      });
    }
    auto const& treefiles() const { return var; }
  };
  struct vesselfile_option {
    std::string var;
    void init(boost::program_options::options_description& desc) {
      desc.add_options()("vesselfile,v",
        boost::program_options::value<std::string>(&var)->required(),
        "file with cylinders describing the hepatic artery tree");
    }
    void parse(boost::program_options::variables_map const&) {}
    auto vesselfile() const { return boost::filesystem::path{var}; }
  };
  struct seed_option {
    std::random_device::result_type var;
    void init(boost::program_options::options_description& desc) {
      desc.add_options()("seed",
        boost::program_options::value<std::random_device::result_type>(&var),
        "random seed value used to initialize URNG");
    }
    void parse(boost::program_options::variables_map const& vm) {
      if (!vm.count("seed"))
        var = std::random_device{}();
      std::cout << "Seed: " << var << std::endl;
    }
    auto const& seed() const { return var; }
  };
  struct data_directory_option {
    std::string var;
    void init(boost::program_options::options_description& desc) {
      desc.add_options()("data-directory,d",
        boost::program_options::value<std::string>(&var)->required(),
        "directory where data files may be found");
    }
    void parse(boost::program_options::variables_map const&) {}
    auto data_directory() const { return boost::filesystem::path{var}; }
  };

  template <typename... Options>
  class options : public Options... {
    using option_list = meta::list<Options...>;
    boost::program_options::options_description desc_;
    boost::program_options::variables_map vm_;
  public:
    options() : desc_{"Allowed options"} {
      desc_.add_options()("help,h", "produce help message");
      meta::for_each(option_list{}, [&](auto a) { decltype(a)::init(desc_); });
    }

    auto& description() { return desc_; }
    auto const& variables() const { return vm_; }

    bool parse(int argc, char const* const argv[]) {
      try {
        boost::program_options::store(
          boost::program_options::parse_command_line(argc, argv, desc_), vm_);

        if (vm_.count("help")) {
          std::cout << desc_ << std::endl;
          return false;
        }
        boost::program_options::notify(vm_);
        meta::for_each(option_list{}, [&](auto a) { decltype(a)::parse(vm_); });
        return true;
      }
      catch (boost::program_options::error const& ex) {
        std::cerr << ex.what() << std::endl;
        return false;
      }
    }
  };

  //TODO: Allow optional unit values
  template <typename U, typename T>
  auto unit_value(boost::units::quantity<U,T>* p, identity_t<T> prefix = T(1)) {
    assert(p);
    return boost::program_options::value<double>()->notifier([=](T const& val) {
       *p = boost::units::quantity<U,T>::from_value(val) * prefix;
    });
  }

  template <typename T>
  auto option_value(boost::optional<T>* p) {
    *p = boost::none;
    return boost::program_options::value<T>()->notifier([=](T const& val) { *p = val; });
  }

  template <typename T, typename U>
  auto option_unit_value(boost::optional<boost::units::quantity<U,T>>* p, identity_t<T> prefix = T(1)) {
    *p = boost::none;
    return boost::program_options::value<T>()->notifier([=](T const& val) {
      *p = boost::units::quantity<U,T>::from_value(val) * prefix;
    });
  }
}//jhmi
#endif

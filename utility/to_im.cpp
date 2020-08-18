#include "utility/volume_image.hpp"
#include <mip/imgio.h>
#include <boost/program_options.hpp>
#include <range/v3/algorithm.hpp>
#include <range/v3/utility/iterator.hpp>
#include <fmt/ostream.h>

namespace po = boost::program_options;
using namespace jhmi;

template <typename T>
void writeIm(std::vector<std::string> const& vimgs) {
  for (auto& vimg : vimgs) {
    volume_image<T> vi{vimg};
    std::vector<float> data;
    //Make the data a contiguous float array
    ranges::copy(vi, ranges::back_inserter(data));
    auto oimg = vimg + ".im";
    ::writeimage(const_cast<char*>(oimg.c_str()),
      vi.width(), vi.height(), vi.depth(), &data[0]);
  }
}

int main(int argc, char* argv[]) {

  try {
    po::options_description desc("Allowed options");
    std::string type;
    std::vector<std::string> vimgs;
    desc.add_options()
      ("help,h", "produce help message")
      ("volume-images", po::value<std::vector<std::string>>(&vimgs)->required(),
       "The volume image to convert")
      ("type", po::value<std::string>(&type)->required(),
       "type may be float32, float64, uint8, or uint32");
    po::positional_options_description p;
    p.add("volume-images", -1);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);

    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 1;
    }
    po::notify(vm);

    if (type == "float32") { writeIm<float>(vimgs); }
    else if (type == "float64") { writeIm<double>(vimgs); }
    else if (type == "uint8") { writeIm<std::uint8_t>(vimgs); }
    else if (type == "uint32") { writeIm<std::uint32_t>(vimgs); }
    else {
      std::cerr << "Invalid format given\n";
      return 1;
    }
  }
  catch (std::exception const& ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
  return 0;
}

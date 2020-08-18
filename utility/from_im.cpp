#include "utility/volume_image.hpp"
#include <mip/imgio.h>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <range/v3/algorithm.hpp>
#include <range/v3/utility/iterator.hpp>
#include <fmt/ostream.h>

namespace fs = boost::filesystem;
namespace po = boost::program_options;
using namespace jhmi;

template <typename T>
void writeBinz(fs::path const& output_path, std::vector<std::string> const& ims) {
  for (auto& im : ims) {
    int x, y, z;
    auto img = readimage3d(const_cast<char*>(im.c_str()), &x, &y, &z);
    volume_image<T> vi{int3{x,y,z}, cube<m3>{m3{}, 4.7951998710632 * int3{x,y,z} * mm}};
    //Make the data a contiguous float array
    std::copy(img, img + x * y * z, vi.begin());

    vi.write(output_path / fs::path{im}.stem().replace_extension(".binz"));
  }
}

int main(int argc, char* argv[]) {

  try {
    po::options_description desc("Allowed options");
    std::string type, output_path;
    std::vector<std::string> vimgs;
    desc.add_options()
      ("help,h", "produce help message")
      ("im-files", po::value<std::vector<std::string>>(&vimgs)->required(),
       "im files to convert")
      ("type", po::value<std::string>(&type)->required(),
       "output type may be float32, float64, uint8, or uint32")
      ("output-path,o", po::value<std::string>(&output_path)->required(),
       "path where convert images will be saved");
    po::positional_options_description p;
    p.add("im-files", -1);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);

    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 1;
    }
    po::notify(vm);

    if (type == "float32") { writeBinz<float>(fs::path{output_path}, vimgs); }
    else if (type == "float64") { writeBinz<double>(fs::path{output_path}, vimgs); }
    else if (type == "uint8") { writeBinz<std::uint8_t>(fs::path{output_path}, vimgs); }
    else if (type == "uint32") { writeBinz<std::uint32_t>(fs::path{output_path}, vimgs); }
    else {
      std::cerr << "Invalid output format given\n";
      return 1;
    }
  }
  catch (std::exception const& ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
  return 0;
}


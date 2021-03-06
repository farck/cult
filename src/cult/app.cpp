#include "./app.h"
#include "./benchcycles.h"
#include "./cpuinfo.h"
#include "./schedutils.h"

namespace cult {

App::App(int argc, char* argv[]) noexcept
  : _cmd(argc, argv),
    _zone(1024 * 32),
    _heap(&_zone),
    _verbose(true),
    _round(true),
    _dump(false),
    _json(&_output) {

  SchedUtils::setAffinity(0);

  if (_cmd.hasKey("--dump")) _dump = true;
  if (_cmd.hasKey("--quiet")) _verbose = false;
  if (_cmd.hasKey("--no-rounding")) _round = false;

  if (verbose()) {
    printf("CULT v%d.%d.%d - CPU Ultimate Latency Test\n",
      CULT_VERSION_MAJOR,
      CULT_VERSION_MINOR,
      CULT_VERSION_MICRO);

    printf("\n");
    printf("Parameters:\n");
    printf("  --dump        - Dump generated asm to stdout\n");
    printf("  --quiet       - Quiet mode, no output except final JSON\n");
    printf("  --no-rounding - Don't round cycles and latencies\n");
    printf("  --output=file - Output to file instead of stdout\n");
    printf("\n");
  }
}

App::~App() noexcept {
}

int App::run() noexcept {
  const asmjit::CpuInfo& hostCpu = asmjit::CpuInfo::getHost();

  _json.openObject();
  _json.beforeRecord()
       .addKey("cult")
       .openObject()
         .beforeRecord()
         .addKey("version").addStringf("%d.%d.%d", CULT_VERSION_MAJOR, CULT_VERSION_MINOR, CULT_VERSION_MICRO)
         .beforeRecord()
         .addKey("cpu").addString(hostCpu.getBrandString())
       .closeObject();

  {
    CpuInfo cpuInfo(this);
    cpuInfo.run();
  }

  {
    BenchCycles benchCycles(this);
    benchCycles.run();
  }

  _json.nl()
       .closeObject()
       .nl();

  const char* outputFileName = _cmd.getKey("--output");
  if (outputFileName) {
    FILE* file = fopen(outputFileName, "wb");
    if (!file) {
      printf("Couldn't open output file: %s\n", outputFileName);
    }
    else {
      fwrite(_output.getData(), _output.getLength(), 1, file);
      fclose(file);
    }
  }
  else {
    puts(_output.getData());
  }
  return 0;
}

} // cult namespace

int main(int argc, char* argv[]) {
  return cult::App(argc, argv).run();
}

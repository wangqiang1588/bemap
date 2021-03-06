#include <iostream>
#include <string>
#include <cmath>
#include <iomanip>

#include "BlackScholes.hpp"

/* Number of iterations */
#define N_ITER 5

/* cl variables */
std::string sourceName("BlackScholes.cl");
std::string compileOptions;

/* Constants */
size_t globalWorkSize[3] = { -1, -1, -1 };
size_t localWorkSize[3]  = { 256, -1, -1 };
size_t nLocals           = 256;
size_t nGlobals          = -1;

cl_device_type deviceType  = CL_DEVICE_TYPE_CPU;
std::string platformId     = "";
kernelVersion kernelVer    = SCALAR;
std::string outputFilename = "";

/* User parameters */
int verbose      = false;
int showPrepTime = false;
int showDevInfo  = false;
int compResult   = false;
int choose       = false;

/* BlackScholes parameters */
int width         = 128;
int optNum        = 10 * 1024 * 1024;
float riskFree    = 0.02f;
float volatility  = 0.30f;
int height;

/* Options long names */
static struct option longopts[] = {
    { "verbose",         no_argument,            NULL,              'v' },
    { "help",            no_argument,            NULL,              'h' },
    { "output",          required_argument,      NULL,              'o' },
    { "optnum",          required_argument,      NULL,              'O' },
    { "riskfree",        required_argument,      NULL,              'R' },
    { "volatility",      required_argument,      NULL,              'V' },
    { "kernel",          required_argument,      NULL,              'k' },
    { "width",           required_argument,      NULL,              'W' },
    { "workitems",       required_argument,      NULL,              'w' },
    { "use-gpu",         no_argument,            NULL,              'g' },
    { "choose-dev",      no_argument,            NULL,              'd' },
    { "choose-plat",     required_argument,      NULL,              'p' },
    { "dev-info",        no_argument,            &showDevInfo,      true},
    { "prep-time",       no_argument,            &showPrepTime,     true},
    { "comp-result",     no_argument,            &compResult,       true},
    { NULL,              0,                      NULL,               0  }
};

void help(const std::string & filename)
{
    std::cout << filename
              << " [--verbose|-v] [--help|-h] [--output|-o FILENAME]" << std::endl
              << "     [--optnum|-O NUMBER] [--riskfree|-R NUMBER] [--volatility|-V NUMBER]" << std::endl 
              << "     [--kernel|-k NUMBER] [--width|-W NUMBER] [--workitems|-w NUMBER]" << std::endl 
              << "     [--use-gpu|-g] [--choose-dev|-d] [--choose-plat|-p DEV]" << std::endl 
              << "     [--dev-info] [--prep-time] [--comp-result]" << std::endl
              << std::endl 
              << "* Options *" 
              << std::endl 
              << " --verbose             Be verbose" << std::endl 
              << " --help                Print this message" << std::endl 
              << " --output=NAME         Write results to this file" << std::endl 
              << " --optnum=NUMBER       Number of elements in the data array -- default = 50 * 1024 * 1024" << std::endl 
              << " --riskfree=NUMBER     The annualized risk-free interest rate, continuously compounded -- default = 0.02" << std::endl 
              << " --volatility=NUMBER   The volatility of stock's returns -- default = 0.30" << std::endl 
              << " --kernel=KERNEL       Kernel mode (0, 1, [2, 3]) -- default = 0" << std::endl 
              << "                                 [0] Scalar" << std::endl 
              << "                                 [1] SIMD = Single Instruction Multiple Data" << std::endl 
              << " --width=NUMBER        Data width (for SIMD mode only, must be a multiple of 8)" << std::endl 
              << " --workitems=NUMBER    Number of (local) workitems for Scalar mode" << std::endl 
              << " --use-gpu             Use GPU as the CL device" << std::endl 
              << " --choose-dev          Choose which OpenCL device to use" << std::endl 
              << " --choose-plat=DEV     Choose which OpenCL platform to use (0, 1, 2)" << std::endl 
              << "                                  [0] Advanced Micro Devices, Inc." << std::endl 
              << "                                  [1] NVIDIA Corporation" << std::endl
              << "                                  [2] Intel(R) Corporation" << std::endl 
              << "                                  default: Any CPU device" << std::endl 
              << " --dev-info            Show Device Info" << std::endl 
              << " --prep-time           Show initialization, memory preparation and copyback time" << std::endl 
              << " --comp-result         Compare GPU and CPU results" << std::endl 
              << std::endl 
              << " * Examples *" << std::endl 
              << filename << " [OPTS...] -v -k 0" << std::endl 
              << filename << " [OPTS...] -v --workitems=128" << std::endl 
              << std::endl;

    exit(0);
}

void option(int ac, char **av)
{
    if (ac == 1) std::cout << av[0] << ": Execute with default parameter(s)..\n(--help for program usage)\n\n";
    int opt;
    while ((opt =
            getopt_long(ac, av, "vho:O:R:V:k:W:w:gdp:T:", longopts,
                        NULL)) != -1) {
        switch (opt) {

        case '?':
            ERROR_HANDLER(0,
                          "Invalid option '" +
                          std::string(av[optind - 1]) + "'");
            break;

        case ':':
            ERROR_HANDLER(0,
                          "Missing argument of option '" +
                          std::string(av[optind - 1]) + "'");
            break;

            /* Verbose */
        case 'v':
            verbose = true;
            break;

            /* Help */
        case 'h':
            help(std::string(av[0]));
            break;

            /* Output to file */
        case 'o':
            outputFilename = std::string(optarg);
            break;

            /* optNum */
        case 'O':
            {
                std::istringstream iss(optarg);
                int a = -1;
                iss >> a;
                optNum = a;
                ERROR_HANDLER((!iss.fail()),
                              "Invalid argument '" + std::string(optarg) +
                              "'");
            }
            break;

            /* riskFree */
        case 'R':
            {
                std::istringstream iss(optarg);
                float a = -1;
                iss >> a;
                riskFree = a;
                ERROR_HANDLER((!iss.fail()),
                              "Invalid argument '" + std::string(optarg) +
                              "'");
            }
            break;

            /* volatility */
        case 'V':
            {
                std::istringstream iss(optarg);
                float a = -1;
                iss >> a;
                volatility = a;
                ERROR_HANDLER((!iss.fail()),
                              "Invalid argument '" + std::string(optarg) +
                              "'");
            }
            break;

            /* Kernel mode */
        case 'k':
            {
                std::istringstream iss(optarg);
                int a = -1;
                iss >> a;
                kernelVer = kernelVersion(a);
                ERROR_HANDLER((!iss.fail()),
                              "Invalid argument '" + std::string(optarg) +
                              "'");
                ERROR_HANDLER((kernelVer >= 0
                               && kernelVer <= 3),
                              "Invalid kernel mode: '" +
                              std::string(optarg) + "'");
            }
            break;


            /* Data width */
        case 'W':
            {
                std::istringstream iss(optarg);
                int a = -1;
                iss >> a;
                width = a;
                ERROR_HANDLER((!iss.fail()),
                              "Invalid argument '" + std::string(optarg) +
                              "'");
                ERROR_HANDLER((width % 8 == 0),
                              "Invalid argument '" + std::string(optarg) +
                              "', must be a multiple of 8");
            }
            break;
            /* Number of workitems */
        case 'w':
            {
                std::istringstream iss(optarg);
                int a = -1;
                iss >> a;
                nLocals = a;
                ERROR_HANDLER((!iss.fail()),
                              "Invalid argument '" + std::string(optarg) +
                              "'");
            }
            break;

            /* Use GPU if invoked */
        case 'g':
            deviceType = CL_DEVICE_TYPE_GPU;
            break;

            /* Choose which OpenCL device to use */
        case 'd':
            {
                choose = true;
                showDevInfo = true;
            }
            break;

            /* Choose which OpenCL platform to use */
        case 'p':
            {
                std::istringstream iss(optarg);
                platforms platformNum;
                int a = -1;
                iss >> a;
                platformNum = platforms(a);
                ERROR_HANDLER((!iss.fail()),
                              "Invalid argument '" + std::string(optarg) +
                              "'");
                ERROR_HANDLER((platformNum >= 0
                               && platformNum <= 2),
                              "Invalid platform: '" + std::string(optarg) +
                              "'");
                platformId = platform[platformNum];
                if (platformNum == NVIDIA)
                    deviceType = CL_DEVICE_TYPE_GPU;
            }
            break;

        case 0:
            break;

        default:
            ERROR_HANDLER(0, "Error: parsing arguments");
        }
    }
}

int main(int argc, char **argv)
{
    /* Parse user input */
    option(argc, argv);
    width = (kernelVer == SIMD) ? width : 1;
    height = (kernelVer == SIMD) ? (optNum / width) : optNum;

    verbose && std::cerr << "BLACK SCHOLES, OpenCL Implementation with " << ((deviceType==CL_DEVICE_TYPE_GPU)?("GPU"):("CPU"))
                         << std::endl << std::endl
                         << "Number of options       = " << optNum << std::endl
                         << "Riskfree rate           = " << riskFree << std::endl
                         << "Volatility              = " << volatility << std::endl
                         << "Number of workitems     = " << ((kernelVer == STSD || kernelVer == STAD)? 1 : nLocals) << std::endl
                         << "Kernel mode             = " << kernelStr[kernelVer] << std::endl
                         << "Data width (SIMD only)  = " << width << std::endl
                         << "Show prep time          = " << ((showPrepTime)?("True"):("False")) << std::endl 
                         << "Show device info        = " << ((showDevInfo)?("True"):("False")) << std::endl
                         << "Compare to CPU results  = " << ((compResult)?("True"):("False")) << std::endl
                         << "Executing .. " << std::endl;

    BlackScholesCL BlackScholesCL(optNum, riskFree, volatility, width,
                                  height, platformId, deviceType,
                                  kernelVer);

    BlackScholesCL.init();

    for (int i = 0; i < N_ITER; i++) {
        BlackScholesCL.prep_memory();
        BlackScholesCL.execute();
        std::cerr << "Iteration " << i+1 << "/" << N_ITER << ": DONE." << std::endl;
        BlackScholesCL.copyback();

        BlackScholesCL.compare_to_cpu();

        outParam output;
        output.outputFilename = outputFilename;
        BlackScholesCL.output(reinterpret_cast < void *>(&output));

        BlackScholesCL.clean_mem();
    }
    BlackScholesCL.finish();

    return 0;
}

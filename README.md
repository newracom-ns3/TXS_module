# (IEEE 802.11be) Triggered TXOP sharing (TXS) mode 1 Simulator (Demo version)
## Table of Contents
* [Overview](#overview)
* [Implementation Method](#implementation-method)
* [Running TXS simulator](#running-txs-simulator)
* [Simulator Validity](#simulator-validity)
* [Notifications](#notifications)

> **NOTE**: The proposed simulator is a demo version. The complete version (including Triggered TXOP sharing mode 2) will be coming soon.

<a name="overview"></a>
## Overview: What is triggered TXOP sharing
Recently, with the emergence of low latency applications such as VR/AR, worst-case delay or reliability is being considered important. However, it is not achievable with the existing MAC features such as a transmission opportunity (TXOP) in IEEE 802.11e amendment, a shared TXOP in IEEE 802.11ac amendment, trigger-based uplink multi-user (UL MU) in IEEE 802.11ax amendment, etc. As one of the alternatives, triggered TXOP sharing was proposed in IEEE 802.11be amendment. Unfortunately, although some simulators such as ns-3, MATLAB, etc provide Wi-Fi simulations, they do not include a portion of IEEE 11be functions, including the triggered TXOP sharing function. Therefore, we implemented a new MAC module that performs the triggered TXOP sharing.

<p align="center"><img width="600" alt="txs_mode_1" src="https://github.com/newracom-ns3/TXS_module/assets/126837751/127d3923-3dba-4654-a71a-e48504bf8fc9">
<p align="center"><em>(a) Triggered TXOP sharing mode 1</em>
<p align="center"><img width="600" alt="txs_mode_2" src="https://github.com/newracom-ns3/TXS_module/assets/126837751/446f80aa-2e38-4730-a02e-02917e6c0a93">
<p align="center"><em>(b) Triggered TXOP sharing mode 2</em>

<em> Figure 1. (a): The shared STA can send one or more non-TB PPDU to the AP; (b): The shared STA can send a PPDU to the peer STA  or send one or more non-TB PPDU to the AP.</em>

Figure 1-(a) shows an example of triggered TXOP sharing mode 1 in which the shared STA can send one or more non-trigger-based PPDU(non-TB PPDU) to the AP.

Figure 2-(b) shows an example of triggered TXOP sharing mode 2 in which the shared STA can send one or more non-TB PPDU to a peer-to-peer STA as well as the AP.

The two modes can release the latency of a shared STA since it has a more frequent transmission opportunity by the AP.

We verify the performance in [Simulator Validity](#simulator-validity) section.

<a name="implementation-method"></a>
## Implementation Method

<p align="center"><img width="600" alt="txs_mode_1" src="https://github.com/newracom-ns3/TXS_module/assets/126837751/83b40355-ef1f-453f-8460-ff95d93a30b8">
<p align="center"><em> Figure 2. MAC implementation architecture for triggered TXOP sharing mode 1.</em>

In development, we tried to ensure that the module could be easily integrated with a later version of ns-3 (3.40) for improved compatibility.

To achieve this, we added a new FrameExchangeManager (FEM) that includes the TXS function (which is called TxsFEM), as shown in  Figure 2. The TxsFEM inherits from the legacy FEMs (EhtFEM, HeFEM, VhtFEM, etc.).

> **NOTE**: Now, the TXS module is not fully compatible with the basis of ns-3 (3.40). We will soon provide the complete version with full compatibility. It means that you will be able to run the triggered TXOP sharing mode 1 in your customized ns-3 simulator by adding our TXS module to your "src" directory. Therefore, We recommend running the TXS module only in our code.

## Running TXS simulator: How to simulate triggered TXOP sharing mode 1
Our simulator provides three simulation scenarios.
- **Baseline EDCA operation**: it is that all STA and AP are competing to get TXOP for transmission.
- **UL MU operation**: it works with the DL MU operation. AP triggers UL MU operation after downlink transmission once. We adopted the round-robin scheme by utilizing the ns-3 scheduler function.
- **Triggered TXOP sharing**: it is based on the UL MU operation scenario, additionally, AP shares its whole remaining TXOP to a specific STA after transmission once. According to IEEE 802.11be specification, AP can select which STA will be shared and when it will be shared. However, we designated the shared STA to one specific STA and time as whole remaining TXOP in this scenario.

Example files for each scenario (example.cc, mu-ul-example.cc, triggered-txs-mode-1-example.cc) are in the directory src/txs-module/examples, 

You can run the examples by typing the following
```shall
:~/TXS_module$ ./ns3 run <the example file name>
```

> **NOTE**: If you use Cmake, must configure the option of Cmake as follows
```shall
:~/TXS_module/cmake-cache$ cmake -DNS3_EXAMPLES=ON ..
```

<a name="simulator-validity"></a>
## Simulator Validity

### Getting simulation results
@cm.lee, please add this section.

### Analysing simulation results

<p align="center"><img width="600" alt="image" src="https://github.com/newracom-ns3/TXS_module/assets/126837751/3404e851-d329-4e1c-884d-b440e9f6aef6">
<p align="center"><em> Figure 3. Considered network topology and simulation scenarios.</em>
  
We measured BSS throughput, shared STA throughput, and shared STA transmission latency according to the three scenarios in [Evaluation Methodology](https://mentor.ieee.org/802.11/dcn/14/11-14-0571-12-00ax-evaluation-methodology.docx). Network topology and simulation scenarios are shown in Figure 3. 

<p align="center"><img width="379" alt="image" src="https://github.com/newracom-ns3/TXS_module/assets/126837751/93948d9a-e633-4721-8a59-95b5b36be511">

The main parameters of the simulation are described in the above Table 1. The parameters are based on the guide document [Simulation Scenarios](https://mentor.ieee.org/802.11/dcn/14/11-14-0980-16-00ax-simulation-scenarios.docx).

All STA associates with AP and transmits the QoS data to AP only. In practical conditions, AP generally has more data to transmit than STA. On the other hand, STAs that utilize UL MU or Triggered TXOP sharing operations are compensated by applying MU EDCA parameters. When the MU EDCA parameters multiplier becomes bigger, the compensation effect is stronger and the STA loses its opportunity relatively to get its own TXOP.

Here, we assumed that there is only one shared STA.

<p align="center"><img width="450" alt="image" src="https://github.com/newracom-ns3/TXS_module/assets/126837751/9c75b3fa-20c7-4afb-942c-1586ece1595f">
  <p align="center"><em> Figure 4. Throughput results for three scenarios: baseline EDCA operation, UL MU, and UL MU+Triggered TXOP sharing</em>
<p align="center"><img width="450" alt="image" src="https://github.com/newracom-ns3/ns-3-newracom/assets/126837751/61f6b3b2-2006-410f-8505-94d2ceec38df">
  <p align="center"><em> Figure 5. Transmission Latency of shared STA for three scenarios: baseline EDCA operation, UL MU, and UL MU+Triggered TXOP sharing.</em>

Figure 4 and Figure 5 show the throughput and average latency of the shared TXS STA. 

These show the outperforms aspects of the throughput and average latency in UL MU+Triggered TXOP sharing. Triggered TXOP sharing gets advantages when the compensation effect from MU EDCA parameters increases. Increasing the influence of MU EDCA parameters means that AP wins more chance to channel access than STAs. Then, the transmission of STAs becomes more dependent on AP. It leads to the performance improvement of the shared STA throughput and average latency in the considered scenarios.

<a name="notifications"></a>
## Notifications
- The provided simulator is a demo version. So, the TXS module is not compatible with your customized ns-3 codes.
    - We recommend running the TXS module only in our code.
- If you contact us, please mail to cm.lee@newratek.com or sm.lee@newratek.com.

---

# The Network Simulator, Version 3

[![codecov](https://codecov.io/gh/nsnam/ns-3-dev-git/branch/master/graph/badge.svg)](https://codecov.io/gh/nsnam/ns-3-dev-git/branch/master/)
[![Gitlab CI](https://gitlab.com/nsnam/ns-3-dev/badges/master/pipeline.svg)](https://gitlab.com/nsnam/ns-3-dev/-/pipelines)
[![Github CI](https://github.com/nsnam/ns-3-dev-git/actions/workflows/per_commit.yml/badge.svg)](https://github.com/nsnam/ns-3-dev-git/actions)

## Table of Contents

* [Overview](#overview-an-open-source-project)
* [Building ns-3](#building-ns-3)
* [Testing ns-3](#testing-ns-3)
* [Running ns-3](#running-ns-3)
* [ns-3 Documentation](#ns-3-documentation)
* [Working with the Development Version of ns-3](#working-with-the-development-version-of-ns-3)
* [Contributing to ns-3](#contributing-to-ns-3)
* [Reporting Issues](#reporting-issues)
* [ns-3 App Store](#ns-3-app-store)

> **NOTE**: Much more substantial information about ns-3 can be found at
<https://www.nsnam.org>

## Overview: An Open Source Project

ns-3 is a free open source project aiming to build a discrete-event
network simulator targeted for simulation research and education.
This is a collaborative project; we hope that
the missing pieces of the models we have not yet implemented
will be contributed by the community in an open collaboration
process. If you would like to contribute to ns-3, please check
the [Contributing to ns-3](#contributing-to-ns-3) section below.

This README excerpts some details from a more extensive
tutorial that is maintained at:
<https://www.nsnam.org/documentation/latest/>

## Building ns-3

The code for the framework and the default models provided
by ns-3 is built as a set of libraries. User simulations
are expected to be written as simple programs that make
use of these ns-3 libraries.

To build the set of default libraries and the example
programs included in this package, you need to use the
`ns3` tool. This tool provides a Waf-like API to the
underlying CMake build manager.
Detailed information on how to use `ns3` is included in the
[quick start guide](doc/installation/source/quick-start.rst).

Before building ns-3, you must configure it.
This step allows the configuration of the build options,
such as whether to enable the examples, tests and more.

To configure ns-3 with examples and tests enabled,
run the following command on the ns-3 main directory:

```shell
./ns3 configure --enable-examples --enable-tests
```

Then, build ns-3 by running the following command:

```shell
./ns3 build
```

By default, the build artifacts will be stored in the `build/` directory.

### Supported Platforms

The current codebase is expected to build and run on the
set of platforms listed in the [release notes](RELEASE_NOTES.md)
file.

Other platforms may or may not work: we welcome patches to
improve the portability of the code to these other platforms.

## Testing ns-3

ns-3 contains test suites to validate the models and detect regressions.
To run the test suite, run the following command on the ns-3 main directory:

```shell
./test.py
```

More information about ns-3 tests is available in the
[test framework](doc/manual/source/test-framework.rst) section of the manual.

## Running ns-3

On recent Linux systems, once you have built ns-3 (with examples
enabled), it should be easy to run the sample programs with the
following command, such as:

```shell
./ns3 run simple-global-routing
```

That program should generate a `simple-global-routing.tr` text
trace file and a set of `simple-global-routing-xx-xx.pcap` binary
PCAP trace files, which can be read by `tcpdump -n -tt -r filename.pcap`.
The program source can be found in the `examples/routing` directory.

## Running ns-3 from Python

If you do not plan to modify ns-3 upstream modules, you can get
a pre-built version of the ns-3 python bindings.

```shell
pip install --user ns3
```

If you do not have `pip`, check their documents
on [how to install it](https://pip.pypa.io/en/stable/installation/).

After installing the `ns3` package, you can then create your simulation python script.
Below is a trivial demo script to get you started.

```python
from ns import ns

ns.LogComponentEnable("Simulator", ns.LOG_LEVEL_ALL)

ns.Simulator.Stop(ns.Seconds(10))
ns.Simulator.Run()
ns.Simulator.Destroy()
```

The simulation will take a while to start, while the bindings are loaded.
The script above will print the logging messages for the called commands.

Use `help(ns)` to check the prototypes for all functions defined in the
ns3 namespace. To get more useful results, query specific classes of
interest and their functions e.g., `help(ns.Simulator)`.

Smart pointers `Ptr<>` can be differentiated from objects by checking if
`__deref__` is listed in `dir(variable)`. To dereference the pointer,
use `variable.__deref__()`.

Most ns-3 simulations are written in C++ and the documentation is
oriented towards C++ users. The ns-3 tutorial programs (`first.cc`,
`second.cc`, etc.) have Python equivalents, if you are looking for
some initial guidance on how to use the Python API. The Python
API may not be as full-featured as the C++ API, and an API guide
for what C++ APIs are supported or not from Python do not currently exist.
The project is looking for additional Python maintainers to improve
the support for future Python users.

## ns-3 Documentation

Once you have verified that your build of ns-3 works by running
the `simple-global-routing` example as outlined in the [running ns-3](#running-ns-3)
section, it is quite likely that you will want to get started on reading
some ns-3 documentation.

All of that documentation should always be available from
the ns-3 website: <https://www.nsnam.org/documentation/>.

This documentation includes:

* a tutorial
* a reference manual
* models in the ns-3 model library
* a wiki for user-contributed tips: <https://www.nsnam.org/wiki/>
* API documentation generated using doxygen: this is
  a reference manual, most likely not very well suited
  as introductory text:
  <https://www.nsnam.org/doxygen/index.html>

## Working with the Development Version of ns-3

If you want to download and use the development version of ns-3, you
need to use the tool `git`. A quick and dirty cheat sheet is included
in the manual, but reading through the Git
tutorials found in the Internet is usually a good idea if you are not
familiar with it.

If you have successfully installed Git, you can get
a copy of the development version with the following command:

```shell
git clone https://gitlab.com/nsnam/ns-3-dev.git
```

However, we recommend to follow the GitLab guidelines for starters,
that includes creating a GitLab account, forking the ns-3-dev project
under the new account's name, and then cloning the forked repository.
You can find more information in the [manual](https://www.nsnam.org/docs/manual/html/working-with-git.html).

## Contributing to ns-3

The process of contributing to the ns-3 project varies with
the people involved, the amount of time they can invest
and the type of model they want to work on, but the current
process that the project tries to follow is described in the
[contributing code](https://www.nsnam.org/developers/contributing-code/)
website and in the [CONTRIBUTING.md](CONTRIBUTING.md) file.

## Reporting Issues

If you would like to report an issue, you can open a new issue in the
[GitLab issue tracker](https://gitlab.com/nsnam/ns-3-dev/-/issues).
Before creating a new issue, please check if the problem that you are facing
was already reported and contribute to the discussion, if necessary.

## ns-3 App Store

The official [ns-3 App Store](https://apps.nsnam.org/) is a centralized directory
listing third-party modules for ns-3 available on the Internet.

More information on how to submit an ns-3 module to the ns-3 App Store is available
in the [ns-3 App Store documentation](https://www.nsnam.org/docs/contributing/html/external.html).

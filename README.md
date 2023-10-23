# Sphinx on the low-end IoT

#### Abstract
*Anonymisation in the low-end IoT is an overlooked topic. Therefore, in my thesis I
presents a design of the Sphinx protocol used in anonymous mix networks for con
strained devices. Following this design, an implementation of Sphinx on the embedded
operating system RIOT is evaluated. The evaluation considers memory usage, runtime
analysis and network performance.
The results show that end-to-end anonymisation
is possible for the low-end IoT but at a high cost in terms of network throughput.
In particular, the importance of hardware-accelerated cryptography is highlighted to
improve the runtime of the protocol operation. Lastly, my thesis points to the need
for further research into the reasons for the high network losses observed.*

## Motivation
As the IoT becomes more widespread [[1](#1)], the security of this technology is
becoming increasingly important. This is particularly true as the data collected
and processed includes personal information such as health data, location data
and individual preferences. In addition, IoT devices are often used to monitor 
and control critical infrastructure and production processes, making them
valuable targets for cyber espionage and sabotage.<br>
Unfortunately, the security of IoT solutions has long been a secondary priority 
leading to adversarial attacks and exploitation [[2](#2), [3](#3), [4](#4), [5](#5)]. Often, vulnerabilities 
arise from insecure communication protocls [[6](#6), [7](#7), [8](#8), [9](#9)]. Even with
secure network protocols, metadata can still be collected revealing the time,
frequency and scope of communications as well as the entities involved.
A high level of privacy and security can be achieved by network anonymisation, 
which obscures the identity of the people or devices involved in the
information exchange. This prevents unauthorised access to personal and sensitive
information in advance. Anonymisation can also improve overall security
by making it more difcult for attackers to track or target specifc individuals
or devices. For example, obscuring the identity and location of IoT devices
can make it more difcult for adversaries to launch targeted attacks or identify
vulnerabilities that could be exploited.<br>
Strong anonymisation comes at the cost of slower network throughput [[10](#10)] and operational 
overhead for network nodes. The constrained hardware characteristics 
of the IoT exacerbate this trade-of. Many IoT devices have limited
processing power, storage capacity and connectivity [[11](#11)]. These constraints
make it challenging to design and deploy efective anonymisation technologies
for the IoT.<br>
Anonymisation technologies have largely been neglected in the IoT. To contribute 
to the development of anonymisation for IoT my thesis investigated the
ability of constrained devices used in the IoT to support strong anonymisation 
techniques. To this end, an adapted design of the Sphinx anonymisation
protocol [[12](#12)] is introduced, implemented and evaluated.

## What are Constrained Devices?
RFC 7228 [[13](#13)] provides a classification of constraint devices used in computer
networks. Due to cost and physical limitations, these constrained nodes lack
some of the features typically found in Internetnodes. The limited power, memory
and processing resources impose strict upper bounds on state, code space
and processing cycles, making optimisation critical.
In addition, the network used is constrained and characterised by limitations
that prevent some of the typical capabilities of the link layers commonly used
on the Internet. Constraints include low achievable bitrate/throughput, high
packet loss and packet loss variability, penalties for using larger packets, limits
on reachability over time, and lack of advanced protocol services.
The constrained device classification is based on the storage capacity for
volatile and non-volatile memory, as detailed in Figure 1.
My thesis focuses on Class 2 constrained devices as they can support a
wider range of network protocol stacks and still have resources available for
applications.

|<img src="https://github.com/ludwit/sphinx-for-IoT/blob/main/assets/constaind.png" width="600" />|
|:--:|
|Fig. 1: Classes of constrained devices (Source: [[13](#13)])|

## What are Mix Networks?
Mix networks were first introduced by David Chaum in 1981 [[14](#14)]. To achieve
anonymity, mix networks route messages through a series of mixes, known as the
path. The sender first cryptographically encapsulates the message, and as the
message travels along the path, each mix partially decrypts it. Some anonymity
is provided as long as at least one mix along the path remains genuine and does
not reveal its secrets to a potential adversary.
Unlike Tor, no persistent connection is established across the network, as
each message is routed through a random selection of mix nodes. Another dif-
ference is that mix nodes add a random delay to messages or send messages
in batches to make traffic analysis more difficult. However, this results in high
latencies, so mix networks are not used for applications that require responsive-
ness, such as web browsing. A typical application of mix networks have been
anonymous email relays such as Mixminion [[15](#15)].
Mix networks have been mostly driven out of real-world applications by the
success of Tor, so little work has been done to integrate constrained devices into them.
However, the downside of high latencies in mix networks comes at a lower
cost in low-end IoT, where many applications do not require responsiveness and
could benefit signifcantly from anonymity. One example would be collecting
and transmitting environmental data using network-capable sensors.

## The Sphinx Protocol

Sphinx is a cryptographic message format proposed by George Danezis and Ian
Goldberg in 2009 [[12](#12)]. It was designed to enable anonymous data exchange
over a mix network. Compared to other cryptographic message formats, Sphinx
has a compact header size and very high security, while remaining fexible and
versatile. Sphinx has all the necessary characteristics to meet the requirements
of a strong mix network. These are as follows:
- **Bitwise unlinkability**: Unless the cryptography is broken, it is impossible
to correlate incoming and outgoing messages at a forwarding network node
by analysing its binary representation. This is the fundamental property
that makes all Sphinx messages indistinguishable to an observer.
- **Hidden path**: No one but the creator of a message knows its entire path,
including its actual length. Also, the position in the path is hidden from
all mix nodes except the frst and last.
- **Integrity**: Active attacks to gain knowledge of the destination or content
of messages by modifying and re-injecting them can be detected.
- **Anonymous replies**: Network nodes can encode Single-Use-Reply-Blocks
(SURBs), which in turn can be used by other nodes to send a message
back to its creator, while preserving the recipient's anonymity. Intermediary
nodes cannot distinguish between reply messages and normal forward
messages. As a result, the anonymity sets of both types of traffic are
combined, resulting in improved security for both.

## Desing
### Goals
The purpose of this design and the following implementation is to evaluate whether
the limited resources of constrained devices are sufcient to support Sphinx.
In this sense, the original Sphinx specifcation and network design have been
tailored to this specific use case. Part of the original design has been simplifed,
while others have been extended for new functionality. The overall design goals
are:
- **Security:** The security level of the original Sphinx specifcation has to be
maintained.
- **Simplicity**: The design should simplify mechanisms and operations wherever possible.
- **Storage Efciency**: Message size and network operation memory overhead
should be kept to a minimum.

### Overview
A major change in this design from the original Sphinx specifcation is that
messages are not forwarded beyond the boundaries of the Sphinx network (i.e.
the wider Internet). This means that the recipient of a Sphinx message must
be a node in the anonymous overlay network, and that any node can send,
forward and receive messages. This limitation is acceptable for constrained
devices, as they usually have few and constant communication partners that
can be included in the overlay network in a scenario where anonymity plays
an important role. On the other hand, the network's security is improved as
anonymity is achieved end-to-end, eliminating the vulnerability of the exit node.
Another major change is the introduction of message acknowledgement. This is
done by including a Single-Use-Reply-Block (SURB) in each message, which the
recipient uses to reply with an acknowledgement immediately.
This design only implements sender anonymity, as the IP address of the
recipient must be known to the sender. However, this is suitable for the IoT,
where communication is mainly concentrated around centralised entities such
as controllers or cloud services, which typically do not benefit from anonymity.
To transmit data anonymously, a sender chooses the nodes through which
the message and acknowledgement will pass and encapsulates the message accordingly.
The mix nodes forward the message until it reaches the intended
recipient. There, the payload is extracted, and the acknowledgement is sent
as a reply. Upon receipt of the reply, the sender acknowledges the message.
Messages that have not been acknowledged within a certain period of time are
retransmitted. After several failed retransmission attempts, a message is discarded. 
The resulting message fow is shown in the Figure 2.

|<img src="https://github.com/ludwit/sphinx-for-IoT/blob/main/assets/message-flow.svg" width="800" />|
|:--:|
|Fig. 2: Network message flow|

## Implementation

### Operating System
The Sphinx implementation has been developed for the RIOT operating system [[16](#16)]. 
RIOT is a lightweight operating system designed for constrained devices
used in the IoT. It runs on processors from 8-bit to 32-bit and comes with a
native port for Linux and MacOS. RIOT is open-source software and supports
the C, C++ and Rust programming languages for applications. RIOT is modular 
and hardware-abstract. This allows RIOT to support many features while
keeping applications memory efficient and easy to develop. RIOT's capabilities
include multiple network stacks, multithreading, real-time capabilities, and it is
party POSIX compliant.

### Crypto Library
The implementation requires a cryptographic library that supports elliptic cryptography 
and symmetric cryptographic functions such as a pseudorandom number 
generator and authentication. The ARM PSA Crypto API [[17](#17)] would be a
good fit, but was under development for RIOT at the time of implementation
[[18](#18)]. Other libraries such as Relic [[19](#19)] and wolfCRYPT [[20](#20)] were discarded due
to their complexity.<br>
This implementation uses the TweetNaCl crypto library [[21](#21)]. TweetNaCl
provides all the necessary functions for this implementation without any parameterisation. 
The advantages are the high level of security and the compactness of
the source code, which fits into 100 tweets, hence the name. Another advantage
is that TweetNaCl is already included in RIOT as a package by default.
Another possible crypto library would have been TinyCrypt [[22](#22)], as it also
supports the necessary functions and focuses on a small memory footprint.

### Integration
Sphinx runs on RIOT as a single thread with a priority just above the main
functions thread. To use Sphinx, a simple command line integration allows one
to start and stop the Sphinx thread as well as to initiate sending a message.
Sphinx also assumes the presence of a public key infrastructure (PKI) to publish the public keys and
addresses of available network nodes. In this implementation, instead of a PKI,
the public keys, private keys and addresses of all nodes are stored in a static
array. The PKI is accessed through a set of global functions tailored to the
needs of the Sphinx implementation.<br>
Sphinx is implemented on top of RIOT's Generic (GNRC) network stack [[23](#23)].
At the link layer, the low-rate wireless personal area network standard IEEE
802.15.4 [[24](#24)] is used as the basis for communication between Sphinx entities. To
enable IPv6 over IEEE 802.15.4, the adaptation layer protocol 6LoWPAN [[25](#25)] 
is used. Then, at the transport layer, UDP is used on top of IPv6. From
an OSI [[26](#26)] perspective, Sphinx can be seen as a shim layer between the transport
and application layer. Note that this implementation does not interoperate with
any application protocol.
Figure 3 shows the active threads on an example board running the RIOT
sphinx application. The network stack becomes visible here, as each layer is
represented by its own thread. The thread implementing IEEE 802.15.4 is
named "at86rf2xx" after the transceiver used in the board.

|<img src="https://github.com/ludwit/sphinx-for-IoT/blob/main/assets/ps.png" width="900" />|
|:--:|
|Fig. 3: Threads running on a Sphinx application in RIOT|

## Evaluation
The experiments were conducted in the FIT IoT-LAB [[27](#27)]. The IoT-LAB provides 
a very large test environment for constrained devices used in the IoT. It
supports 18 different boards and consists of over 1500 nodes spread across six
sites.

### Hardware
All examined boards support an IEEE 802.15.4 radio
interface for low-power, short-range wireless communication. The boards are
further specifed by their embedded processor, volatile memory capacity and
non-volatile memory capacity:
- **SAMR21 Xplained Pro**
    - ARM Cortex-M0+
    - 32 kB RAM
    - 256 kB ROM
- **IoT-LAB M3**
    - ARM Cortex M3
    - 64 kB RAM
    - 256 kB ROM
- **Nordic nRF52840DK**
    - ARM Cortex M4
    - 256 kB RAM
    - 1000 kB ROM

### Runtime
In order to quantify the performance of Sphinx, a runtime measurement was
made considering the creation and processing of a Sphinx message. To do this, a
timestamp of the current system time was taken at the beginning and end of the
functions in question. Then the difference was taken and printed to the console.
Other parts of the program, such as networking, were not included in the runtime
measurement because their impact on the overall runtime was negligible. The
path lengths considered range from six to ten. Figure 4 shows the time taken to create
a message for a given path length.



## References
<a id="1">[1]</a>
McKinsey & Company. What is the Internet of Things?
https://www.mckinsey.com/featured-insights/mckinsey-explainers/what-is-the-internet-of-things , 2022.
<br><br>
<a id="2">[2]</a>
CERT Vulnerability Notes Database. Rsi video technologies videofied security system frontel software uses an insecure custom protocol. https://www.kb.cert.org/vuls/id/792004 , 2015.
<br><br>
<a id="3">[3]</a>
Omar Alrawi, Chaz Lever, Manos Antonakakis, and Fabian Monrose. Sok:
Security evaluation of home-based iot deployments. In 2019 IEEE Symposium
on Security and Privacy (SP) , pages 1362-1380, 2019.
<br><br>
<a id="4">[4]</a>
Catalin Cimpanu. Microsoft: Russian state hackers are using IoT devices to
breach enterprise networks. https://www.zdnet.com/article/microsoft-russian-state-hackers-are-using-iot-devices-to-breach-enterprise-networks, 2019.
<br><br>
<a id="5">[5]</a>
Sarthak Grover and Nick Feamster. The internet of unpatched things.
https://www.youtube.com/watch?v=-778aD_XVKI , 2016.
<br><br>
<a id="6">[6]</a>
Xianghui Cao, Devu Manikantan Shila, Yu Cheng, Zequ Yang, Yang Zhou,
and Jiming Chen. Ghost-in-zigbee: Energy depletion attack on zigbee-
based wireless networks. IEEE Internet of Things Journal , pages 816-829,
2016.
<br><br>
<a id="7">[7]</a>
Francesca Meneghello, Matteo Calore, Daniel Zucchetto, Michele Polese,
and Andrea Zanella. Iot: Internet of threats? A survey of practical security
vulnerabilities in real IoT devices. IEEE Internet of Things Journal , pages
8182-8201, 2019.
<br><br>
<a id="8">[8]</a>
Olayemi Olawumi, Keijo Haataja, Mikko Asikainen, Niko Vidgren, and
Pekka Toivanen. Three practical attacks against zigbee security: Attack
scenario definitions, practical experiments, countermeasures, and lessons
learned. In 2014 14th International Conference on Hybrid Intelligent Systems, 
pages 199-206, 2014.
<br><br>
<a id="9">[9]</a>
Eyal Ronen and Adi Shamir. Extended functionality attacks on IoT devices:
The case of smart lights. In 2016 IEEE European Symposium on Security
and Privacy, pages 3-12, 2016.
<br><br>
<a id="10">[10]</a>
John Geddes. Privacy and Performance Trade-offs in Anonymous Communication Networks. 
Retrieved from the University of Minnesota Digital Conservancy, 
https://hdl.handle.net/11299/185611, 2017.
<br><br>
<a id="11">[11]</a>
IF. Constraints of Internet of Things devices. 
https://securingiot.projectsbyif.com/constraints-of-internet-of-things-devices,
2017.
<br><br>
<a id="12">[12]</a>
George Danezis and Ian Goldberg. Sphinx: A compact and provably secure
mix format. In 2009 30th IEEE Symposium on Security and Privacy , pages
269-282, 2009.
<br><br>
<a id="13">[13]</a>
Carsten Bormann, Mehmet Ersue, and Ari Keränen. Terminology for Constrained-Node Networks. 
RFC 7228, 2014.
<br><br>
<a id="14">[14]</a>
David L. Chaum. Untraceable electronic mail, return addresses, and digital
pseudonyms. Commun. ACM , page 84-90, 1981.
<br><br>
<a id="15">[15]</a>
G. Danezis, R. Dingledine, and N. Mathewson. Mixminion: Design of a
Type III Anonymous Remailer Protocol. In 2003 Symposium on Security and
Privacy, pages 2-15, 2003.
<br><br>
<a id="16">[16]</a>
Emmanuel Baccelli, Cenk Gündoğan, Oliver Hahm, Peter Kietzmann, Martine S. Lenders, 
Hauke Petersen, Kaspar Schleiser, Thomas C. Schmidt,
and Matthias Wählisch. Riot: An open source operating system for low-end 
embedded devices in the IoT. IEEE Internet of Things Journal , pages
4428-4440, 2018.
<br><br>
<a id="17">[17]</a>
Arm Limited. https://armmbed.github.io/mbed-crypto/html/index.html, 2022.
<br><br>
<a id="18">[18]</a>
Lena Boeckmann. Psa crypto api implementation. https://github.com/RIOT-OS/RIOT/pull/18547, 2022.
<br><br>
<a id="19">[19]</a>
Diego F. Aranha. Relic. https://github.com/relic-toolkit/relic
<br><br>
<a id="20">[20]</a>
wolfSSL Inc. wolfcrypt embedded crypto engine. https://www.wolfssl.com/products/wolfcrypt
<br><br>
<a id="21">[21]</a>
Daniel J. Bernstein, Bernard van Gastel, Wesley Janssen, Tanja Lange,
Peter Schwabe, and Sjaak Smetsers. Tweetnacl: A crypto library in 100
tweets. In Progress in Cryptology - LATINCRYPT 2014 , pages 64-83, 2015.
<br><br>
<a id="22">[22]</a>
Intel Corporation. Tinycrypt cryptographic library. https://github.com/intel/tinycrypt , 2017.
<br><br>
<a id="23">[23]</a>
M. Lenders. Analysis and comparison of embedded network stacks. http://doc.riot-os.org/mlenders_msc.pdf, 2016.
<br><br>
<a id="24">[24]</a>
IEEE standard for low-rate wireless networks. IEEE Std 802.15.4-2020
vision of IEEE Std 802.15.4-2015), pages 1-800, 2020.
<br><br>
<a id="25">[25]</a>
Gabriel Montenegro, Jonathan Hui, David Culler, and Nandakishore
Kushalnagar. Transmission of IPv6 Packets over IEEE 802.15.4 Networks.
RFC 4944, 2007.
<br><br>
<a id="26">[26]</a>
Information technology Open Systems Interconnection Basic Reference Model: 
The Basic Model. https://www.iso.org/standard/20269.html , 1994.
<br><br>
<a id="27">[27]</a>
Cedric Adjih, Emmanuel Baccelli, Eric Fleury, Gaetan Harter, Nathalie
Mitton, Thomas Noel, Roger Pissard-Gibollet, Frederic Saint-Marcel, Guillaume 
Schreiner, Julien Vandaele, and Thomas Watteyne. Fit iot-lab: A
large scale open experimental IoT testbed. In 2015 IEEE 2nd World Forum
on Internet of Things (WF-IoT), pages 459-464, 2015.

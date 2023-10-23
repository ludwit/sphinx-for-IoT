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
As the IoT becomes more widespread [[1]](#1), the security of this technology is
becoming increasingly important. This is particularly true as the data collected
and processed includes personal information such as health data, location data
and individual preferences. In addition, IoT devices are often used to monitor 
and control critical infrastructure and production processes, making them
valuable targets for cyber espionage and sabotage.<br>
Unfortunately, the security of IoT solutions has long been a secondary priority 
leading to adversarial attacks and exploitation. Often, vulnerabilities 
arise from insecure communication protocls. Even with
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
Strong anonymisation comes at the cost of slower network throughput and operational 
overhead for network nodes. The constrained hardware characteristics 
of the IoT exacerbate this trade-of. Many IoT devices have limited
processing power, storage capacity and connectivity. These constraints
make it challenging to design and deploy efective anonymisation technologies
for the IoT.<br>
Anonymisation technologies have largely been neglected in the IoT. To contribute 
to the development of anonymisation for IoT my thesis investigated the
ability of constrained devices used in the IoT to support strong anonymisation 
techniques. To this end, an adapted design of the Sphinx anonymisation
protocol is introduced, implemented and evaluated.

## Constrained Devices Definition
RFC 7228 provides a classification of constraint devices used in computer
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
volatile and non-volatile memory, as detailed in the following table.

     +-------------+-----------------------+-------------------------+
     | Name        | data size (e.g., RAM) | code size (e.g., Flash) |
     +-------------+-----------------------+-------------------------+
     | Class 0, C0 | << 10 KiB             | << 100 KiB              |
     |             |                       |                         |
     | Class 1, C1 | ~ 10 KiB              | ~ 100 KiB               |
     |             |                       |                         |
     | Class 2, C2 | ~ 50 KiB              | ~ 250 KiB               |
     +-------------+-----------------------+-------------------------+
     Source: RFC 7228

My thesis focuses on Class 2 constrained devices as they can support a
wider range of network protocol stacks and still have resources available for
applications.

## Overview on Mix Networks
Mix networks were first introduced by David Chaum in 1981. To achieve
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
anonymous email relays such as Mixminion.
Mix networks have been mostly driven out of real-world applications by the
success of Tor, so little work has been done to integrate constrained devices into them.
However, the downside of high latencies in mix networks comes at a lower
cost in low-end IoT, where many applications do not require responsiveness and
could benefit signifcantly from anonymity. One example would be collecting
and transmitting environmental data using network-capable sensors.

## Sphinx Features

Sphinx is a cryptographic message format proposed by George Danezis and Ian
Goldberg in 2009. It was designed to enable anonymous data exchange
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

## Desing Goals
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

## Desing Overview
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
The resulting message fow is shown in the Figure below.

<center>
     <img src="https://github.com/ludwit/sphinx-for-IoT/blob/main/assets/message-flow.svg" width="800" />
</center>

## References
<a id="1">[1]</a>
McKinsey & Company. What is the Internet of Things?
https://www.mckinsey.com/featured-insights/mckinsey-explainers/what-is-the-internet-of-things , 2022.

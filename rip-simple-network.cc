// Network topology
//Routing Information Protocol (RIP) - Distance Vector Routing Protocol
//ns3

//    SRC
//     |<=== source network
//     A-----B
//      \   / \   all networks have cost 1, except
//       \ /  |   for the direct link from C to D, which
//        C  /    has cost 10
//        | /
//        |/
//        D
//        |<=== target network
//       DST
// Two paths SRC-> A -> B -> D (1+1+1=3)
// Another path SRC-> A->C->D->DST (1+1+10=12)
// A, B, C and D are RIPng routers.
// A and D are configured with static addresses.
// SRC and DST will exchange packets.
//
// After about 3 seconds, the topology is built, and Echo Reply will be received.
// After 40 seconds, the link between B and D will break, causing a route failure.
// After 44 seconds from the failure, the routers will recovery from the failure.
// Split Horizoning should affect the recovery time, but it is not. See the manual
// for an explanation of this effect.
//
// If "showPings" is enabled, the user will see:
// 1) if the ping has been acknowledged
// 2) if a Destination Unreachable has been received by the sender
// 3) nothing, when the Echo Request has been received by the destination but
//    the Echo Reply is unable to reach the sender.
// Examining the .pcap files with Wireshark can confirm this effect.

#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/animation-interface.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/ipv4-static-routing-helper.h"
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RipSimpleRouting");

// Global pointer to animation interface
AnimationInterface* g_anim = nullptr;

void TearDownLink(Ptr<Node> nodeA, Ptr<Node> nodeB, uint32_t interfaceA, uint32_t interfaceB)
{
    nodeA->GetObject<Ipv4>()->SetDown(interfaceA);
    nodeB->GetObject<Ipv4>()->SetDown(interfaceB);
    
    // Visualize link failure in animation
    if (g_anim) {
        std::string description = "Link Down";
        g_anim->UpdateNodeColor(nodeA, 255, 0, 0); // Red for failed node
        g_anim->UpdateNodeColor(nodeB, 255, 0, 0);
        g_anim->UpdateNodeDescription(nodeA, description);
        g_anim->UpdateNodeDescription(nodeB, description);
    }
}

void RecoverLink(Ptr<Node> nodeA, Ptr<Node> nodeB, uint32_t interfaceA, uint32_t interfaceB)
{
    nodeA->GetObject<Ipv4>()->SetUp(interfaceA);
    nodeB->GetObject<Ipv4>()->SetUp(interfaceB);
    
    // Visualize link recovery in animation
    if (g_anim) {
        std::string description = "Link Up";
        g_anim->UpdateNodeColor(nodeA, 0, 255, 0); // Green for recovered node
        g_anim->UpdateNodeColor(nodeB, 0, 255, 0);
        g_anim->UpdateNodeDescription(nodeA, description);
        g_anim->UpdateNodeDescription(nodeB, description);
    }
}

int main(int argc, char** argv)
{   
    bool verbose = false;
    bool printRoutingTables = false;
    bool showPings = false;
    std::string SplitHorizon("NoSplitHorizon");

    CommandLine cmd(__FILE__);
    cmd.AddValue("verbose", "turn on log components", verbose);
    cmd.AddValue("printRoutingTables", "Print routing tables at 30, 60 and 90 seconds", printRoutingTables);
    cmd.AddValue("showPings", "Show Ping6 reception", showPings);
    cmd.AddValue("splitHorizonStrategy", 
                 "Split Horizon strategy to use (NoSplitHorizon, SplitHorizon, PoisonReverse)",
                 SplitHorizon);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_NODE));
        LogComponentEnable("RipSimpleRouting", LOG_LEVEL_INFO);
        LogComponentEnable("Rip", LOG_LEVEL_ALL);
        LogComponentEnable("Ipv4Interface", LOG_LEVEL_ALL);
        LogComponentEnable("Icmpv4L4Protocol", LOG_LEVEL_ALL);
        LogComponentEnable("Ipv4L3Protocol", LOG_LEVEL_ALL);
        LogComponentEnable("ArpCache", LOG_LEVEL_ALL);
        LogComponentEnable("Ping", LOG_LEVEL_ALL);
    }

    // Configure split horizon strategy
    if (SplitHorizon == "NoSplitHorizon")
    {
        Config::SetDefault("ns3::RipNg::SplitHorizon", EnumValue(RipNg::NO_SPLIT_HORIZON));
    }
    else if (SplitHorizon == "SplitHorizon")
    {
        Config::SetDefault("ns3::RipNg::SplitHorizon", EnumValue(RipNg::SPLIT_HORIZON));
    }
    else
    {
        Config::SetDefault("ns3::RipNg::SplitHorizon", EnumValue(RipNg::POISON_REVERSE));
    }

    // Create nodes
    NS_LOG_INFO("Create nodes.");
    Ptr<Node> src = CreateObject<Node>();
    Names::Add("SrcNode", src);
    Ptr<Node> dst = CreateObject<Node>();
    Names::Add("DstNode", dst);
    Ptr<Node> a = CreateObject<Node>();
    Names::Add("RouterA", a);
    Ptr<Node> b = CreateObject<Node>();
    Names::Add("RouterB", b);
    Ptr<Node> c = CreateObject<Node>();
    Names::Add("RouterC", c);
    Ptr<Node> d = CreateObject<Node>();
    Names::Add("RouterD", d);

    NodeContainer net1(src, a);
    NodeContainer net2(a, b);
    NodeContainer net3(a, c);
    NodeContainer net4(b, c);
    NodeContainer net5(c, d);
    NodeContainer net6(b, d);
    NodeContainer net7(d, dst);
    NodeContainer routers(a, b, c, d);
    NodeContainer nodes(src, dst);

    // Create channels with different delays
    NS_LOG_INFO("Create channels.");
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", DataRateValue(5000000));
    
    // Set different delays for different paths
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
    NetDeviceContainer ndc1 = csma.Install(net1);
    
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(3)));
    NetDeviceContainer ndc2 = csma.Install(net2);
    
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(4)));
    NetDeviceContainer ndc3 = csma.Install(net3);
    
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
    NetDeviceContainer ndc4 = csma.Install(net4);
    
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(5)));
    NetDeviceContainer ndc5 = csma.Install(net5);
    
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
    NetDeviceContainer ndc6 = csma.Install(net6);
    
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
    NetDeviceContainer ndc7 = csma.Install(net7);

    // Configure routing
    NS_LOG_INFO("Create IPv4 and routing");
    RipHelper ripRouting;

    // Configure RIP interfaces
    ripRouting.ExcludeInterface(a, 1);
    ripRouting.ExcludeInterface(d, 3);

    // Set different metrics
    ripRouting.SetInterfaceMetric(c, 3, 10);
    ripRouting.SetInterfaceMetric(d, 1, 10);
    ripRouting.SetInterfaceMetric(b, 2, 5);
    ripRouting.SetInterfaceMetric(c, 1, 5);

    Ipv4ListRoutingHelper listRH;
    listRH.Add(ripRouting, 0);

    InternetStackHelper internet;
    internet.SetIpv6StackInstall(false);
    internet.SetRoutingHelper(listRH);
    internet.Install(routers);

    InternetStackHelper internetNodes;
    internetNodes.SetIpv6StackInstall(false);
    internetNodes.Install(nodes);

    // Assign IP addresses
    NS_LOG_INFO("Assign IPv4 Addresses.");
    Ipv4AddressHelper ipv4;

    ipv4.SetBase(Ipv4Address("10.0.0.0"), Ipv4Mask("255.255.255.0"));
    Ipv4InterfaceContainer iic1 = ipv4.Assign(ndc1);

    ipv4.SetBase(Ipv4Address("10.0.1.0"), Ipv4Mask("255.255.255.0"));
    Ipv4InterfaceContainer iic2 = ipv4.Assign(ndc2);

    ipv4.SetBase(Ipv4Address("10.0.2.0"), Ipv4Mask("255.255.255.0"));
    Ipv4InterfaceContainer iic3 = ipv4.Assign(ndc3);

    ipv4.SetBase(Ipv4Address("10.0.3.0"), Ipv4Mask("255.255.255.0"));
    Ipv4InterfaceContainer iic4 = ipv4.Assign(ndc4);

    ipv4.SetBase(Ipv4Address("10.0.4.0"), Ipv4Mask("255.255.255.0"));
    Ipv4InterfaceContainer iic5 = ipv4.Assign(ndc5);

    ipv4.SetBase(Ipv4Address("10.0.5.0"), Ipv4Mask("255.255.255.0"));
    Ipv4InterfaceContainer iic6 = ipv4.Assign(ndc6);

    ipv4.SetBase(Ipv4Address("10.0.6.0"), Ipv4Mask("255.255.255.0"));
    Ipv4InterfaceContainer iic7 = ipv4.Assign(ndc7);

    // Configure static routes
    Ptr<Ipv4StaticRouting> staticRouting;
    staticRouting = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting>(
        src->GetObject<Ipv4>()->GetRoutingProtocol());
    staticRouting->SetDefaultRoute("10.0.0.2", 1);
    
    staticRouting = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting>(
        dst->GetObject<Ipv4>()->GetRoutingProtocol());
    staticRouting->SetDefaultRoute("10.0.6.1", 1);

    // Print routing tables
    if (!printRoutingTables)
    {
        Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>(&std::cout);

        Ipv4RoutingHelper::PrintRoutingTableAt(Seconds(30.0), a, routingStream);
        Ipv4RoutingHelper::PrintRoutingTableAt(Seconds(30.0), b, routingStream);
        Ipv4RoutingHelper::PrintRoutingTableAt(Seconds(30.0), c, routingStream);
        Ipv4RoutingHelper::PrintRoutingTableAt(Seconds(30.0), d, routingStream);

        Ipv4RoutingHelper::PrintRoutingTableAt(Seconds(60.0), a, routingStream);
        Ipv4RoutingHelper::PrintRoutingTableAt(Seconds(60.0), b, routingStream);
        Ipv4RoutingHelper::PrintRoutingTableAt(Seconds(60.0), c, routingStream);
        Ipv4RoutingHelper::PrintRoutingTableAt(Seconds(60.0), d, routingStream);

        Ipv4RoutingHelper::PrintRoutingTableAt(Seconds(90.0), a, routingStream);
        Ipv4RoutingHelper::PrintRoutingTableAt(Seconds(90.0), b, routingStream);
        Ipv4RoutingHelper::PrintRoutingTableAt(Seconds(90.0), c, routingStream);
        Ipv4RoutingHelper::PrintRoutingTableAt(Seconds(90.0), d, routingStream);
    }

    // Create ping application
    NS_LOG_INFO("Create Applications.");
    uint32_t packetSize = 1024;
    Time interPacketInterval = Seconds(1.0);
    PingHelper ping(Ipv4Address("10.0.6.2"));

    ping.SetAttribute("Interval", TimeValue(interPacketInterval));
    ping.SetAttribute("Size", UintegerValue(packetSize));
    if (!showPings)
    {
        ping.SetAttribute("VerboseMode", EnumValue(Ping::VerboseMode::VERBOSE));
    }
    ApplicationContainer apps = ping.Install(src);
    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(110.0));

    // Enable traces
    AsciiTraceHelper ascii;
    csma.EnableAsciiAll(ascii.CreateFileStream("rip-simple-routing.tr"));
    csma.EnablePcapAll("rip-simple-routing", true);

    // Configure animation
    AnimationInterface anim("rip-simple-routing-" + SplitHorizon + ".xml");
    g_anim = &anim;  // Store animation interface pointer
    
    // Position nodes
    anim.SetConstantPosition(src, 0.0, 0.0);
    anim.SetConstantPosition(a, 2.0, 1.0);
    anim.SetConstantPosition(b, 4.0, 0.0);
    anim.SetConstantPosition(c, 2.0, -1.0);
    anim.SetConstantPosition(d, 6.0, 0.0);
    anim.SetConstantPosition(dst, 8.0, 0.0);

    // Set node descriptions
    anim.UpdateNodeDescription(a, "Router A\n" + SplitHorizon);
    anim.UpdateNodeDescription(b, "Router B\n" + SplitHorizon);
    anim.UpdateNodeDescription(c, "Router C\n" + SplitHorizon);
    anim.UpdateNodeDescription(d, "Router D\n" + SplitHorizon);

    // Set up link failures and recoveries
    Simulator::Schedule(Seconds(40), &TearDownLink, b, d, 3, 2);
    Simulator::Schedule(Seconds(60), &TearDownLink, c, d, 2, 1);
    
    // Schedule link recoveries
    Simulator::Schedule(Seconds(80), &RecoverLink, b, d, 3, 2);
    Simulator::Schedule(Seconds(100), &RecoverLink, c, d, 2, 1);

    NS_LOG_INFO("Run Simulation.");
    Simulator::Stop(Seconds(131.0));
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");
    
    g_anim = nullptr;  // Clear animation interface pointer
    return 0;
}

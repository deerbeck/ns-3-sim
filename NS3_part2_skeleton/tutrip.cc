//
// Created by developer on 7/6/20.
//
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include <ns3/ipv4-l3-protocol.h>

#include <fstream>
#include <iostream>
#include <json/json.h>
#include <string>
#include <unordered_map>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("SimpleGlobalRoutingExample");

void
TraceRtt(Ptr<OutputStreamWrapper> stream, unsigned short packetSize, Time rtt)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << packetSize << " bytes\t"
                         << rtt.GetMilliSeconds() << " ms" << std::endl;
}

int
main(int argc, char* argv[])
{
    std::string router_identifier = "router";
    std::string host_identifier = "host";
    // Set up some default values for the simulation.  Use the
    Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(210));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue("448kb/s"));

    /*
     * Get a handle to the top level of the JSON file.
     */
    Json::Value root;
    std::ifstream config_file;
    config_file.exceptions(std::ios::failbit | std::ios::badbit);
    config_file.open("./scratch/topology.json", std::ifstream::binary);
    std::cout << "Opening succeeded: " << config_file.is_open() << " Is bad? " << config_file.bad()
              << " Is fail? " << config_file.fail() << std::endl;
    config_file >> root;
    /*
     * Get handles to the different entities of the JSON file. Those are:
     *   - nodes
     *   - links
     *   - flows
     *   - failures
     * IMPORTANT: If you want to use your own JSON files make sure that the host
     * is listed as the source of the link. In addition, nodes in this implementation
     * are restricted to have *exactly one* up-link to the backbone network. Having
     * more connections is possible but would make thinkgs more complicated, since you
     * would have to specify the interface/network address of the host that should be used.
     */
    Json::Value j_nodes = root["topo"]["nodes"];
    Json::Value j_links = root["topo"]["links"];
    Json::Value j_flows = root["flows"];
    Json::Value j_fails = root["failures"];

    /*
     * Create the nodes in the network. Define node containers for:
     *    - all nodes
     *    - the hosts
     *    - the routers.
     * Create all hosts in the network
     */
    NS_LOG_INFO("Create nodes.");

    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Your code goes here
    std::cout << "Create " << j_nodes.size() << " nodes." << std::endl;
    NodeContainer all_nodes, c_hosts, c_router;
    for (int i = 0; i < (int)j_nodes.size(); i++)
    {
        Ptr<Node> node = CreateObject<Node>();
        all_nodes.Add(node);
        if (j_nodes[i]["type"].asString() == router_identifier)
        {
            c_router.Add(node);
        }
        else if (j_nodes[i]["type"].asString() == host_identifier)
        {
            c_hosts.Add(node);
        }
    }
    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

    /* Create a mapping of node ids to nodes in the node container.
     * The name of the node in the JSON file has to map to a node created in the node
     * container. The nodes in the node container do not yet have any semantics to
     * them, yet. They are just plain nodes objects. The semantics (that is, links, applications,
     * etc.) are added to the nodes during the rest of this example.
     *
     * Also distinguish between host and router nodes. Separate the nodes based on the type
     * field of the configuration file.
     */
    std::unordered_map<std::string, Ptr<Node>> node_map;
    std::unordered_map<std::string, Ptr<Node>> routers;
    std::unordered_map<std::string, Ptr<Node>> hosts;
    for (int i = 0; i < (int)j_nodes.size(); i++)
    {
        std::cout << "Create node " << j_nodes[i]["id"].asString() << std::endl;
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Your code goes here
        std::string id = j_nodes[i]["id"].asString();
        node_map[id] = all_nodes.Get(i);

        if (j_nodes[i]["type"].asString() == router_identifier)
        {
            routers[id] = all_nodes.Get(i);
        }
        else if (j_nodes[i]["type"].asString() == host_identifier)
        {
            hosts[id] = all_nodes.Get(i);
        }
        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    }

    /*
     * Use the previously created mapping to create node containers that represent
     * the links in the network topology. Use the identifiers of the nodes to
     * create a corresponding identifier for the edges.
     *
     * In addition, note the identifier for the gateway of the hosts. That is,
     * remember the name of the node the hosts are connected to. Those are later
     * required for the configuration of the routing.
     */
    std::unordered_map<std::string, NodeContainer> edges;
    for (int i = 0; i < (int)j_links.size(); i++)
    {
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Your code goes here
        std::cout << "Create link between " << j_links[i]["source"].asString() << " and "
                  << j_links[i]["target"].asString() << std::endl;
        std::string source = j_links[i]["source"].asString();
        std::string target = j_links[i]["target"].asString();
        NodeContainer nc;
        nc.Add(node_map[source]);
        nc.Add(node_map[target]);
        edges[source + target] = nc;
        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    }

    /*
     * Create the physical channels (point-to-point links, duplex). Iterate over the
     * links in the JSON file and set the data-rate and the delay of the links
     * correspondingly.
     * Store the created devices in a map that maps a string identifier to the
     * net device. Should be the same identifier you created for the edges.
     *
     * Keep in mind that the first interface on every node corresponds to the loopback
     * interface. Thus, interfaces to other nodes start at 1.
     *
     * The interface numbers are later needed to schedule link failure events. The
     * devices are required for the internet addressing.
     *
     * Tip: Keep in mind that the links are duplex, i.e., the graph in the JSON
     * file is undirected!
     */
    PointToPointHelper p2p;
    std::unordered_map<std::string, NetDeviceContainer> device_map;
    std::unordered_map<std::string, std::unordered_map<std::string, int>> iface_nums;
    for (int i = 0; i < (int)j_links.size(); i++)
    {
        std::stringstream key;
        std::string u = j_links[i]["source"].asString();
        std::string v = j_links[i]["target"].asString();
        key << u << v;
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Your code goes here
        std::cout << "Create link between " << u << " and " << v << std::endl;
        p2p.SetDeviceAttribute("DataRate", StringValue(j_links[i]["DataRate"].asString()));
        p2p.SetChannelAttribute("Delay", StringValue(j_links[i]["Delay"].asString()));
        NetDeviceContainer devices = p2p.Install(edges[u + v]);
        device_map[u + v] = devices;
        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

        if (iface_nums.find(u) == iface_nums.end())
        {
            std::unordered_map<std::string, int> target;
            // THe first if index is loopback, thus start with one.
            target.emplace(v, 1);
            iface_nums.emplace(u, target);
        }
        else
        {
            iface_nums.at(u).emplace(v, iface_nums.at(u).size() + 1);
        }
        if (iface_nums.find(v) == iface_nums.end())
        {
            std::unordered_map<std::string, int> target;
            // THe first if index is loopback, thus start with one.
            target.emplace(u, 1);
            iface_nums.emplace(v, target);
        }
        else
        {
            iface_nums.at(v).emplace(u, iface_nums.at(v).size() + 1);
        }
    }

    /*
     * Configure the RIP routing.
     */
    RipHelper ripRouting;
    /*
     * Exclude all host interfaces from the RIP routing. The host interfaces will
     * be configured with static default routes.
     */
    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Your code goes here   
    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

    /*
     * Set the routing metric for RIP links. weight to the link latency.
     */
    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Your code goes here
    for (int i = 0; i < (int)j_links.size(); i++)
    {
        std::cout << "Set metric for link " << j_links[i]["source"].asString() << " to "
                  << j_links[i]["target"].asString() << std::endl;
        std::string source = j_links[i]["source"].asString();
        std::string target = j_links[i]["target"].asString();
        double latency = j_links[i]["Delay"].asString();
        ripRouting.SetInterfaceMetric(node_map[source], iface_nums[source][target], latency);
        ripRouting.SetInterfaceMetric(node_map[target], iface_nums[target][source], latency);
    }
    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    /*
     * Add the routing helper to the list routing with highest priority.
     */
    Ipv4ListRoutingHelper listRH;
    listRH.Add(ripRouting, 0);

    /*
     * Install the internet stack on all nodes.
     */
    InternetStackHelper internet;
    internet.SetRoutingHelper(listRH);
    internet.Install(c_router);

    InternetStackHelper internet2;
    internet2.Install(c_hosts);

    /*
     * Assign the IP addresses. Store the interface container in a mapping
     * that associates the source with the container.
     * Note that the nodes that serve as traffic sink *MUST* be set as
     * source of its up-link to the inter-connect. This makes the configuration
     * of the applications easier. The network address is then always at location 0
     * of the interface container. Else, you would need to store the network addresses
     * as well. Its easy for hosts, but more difficult for nodes with multiple links.
     * For the purpose of this tutorial we keep it thus simple.
     */
    Ipv4AddressHelper ipv4;
    std::unordered_map<std::string, Ipv4InterfaceContainer> out_iface;
    std::unordered_map<std::string, ns3::Ipv4Address> gateways;
    for (int i = 0; i < (int)j_links.size(); i++)
    {
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Your code goes here
        ipv4.SetBase(Ipv4Address(j_links[i]["Network"].asString().c_str()), "255.255.255.0");
        Ipv4InterfaceContainer interfaces = ipv4.Assign(
            device_map[j_links[i]["source"].asString() + j_links[i]["target"].asString()]);
        out_iface[j_links[i]["source"].asString()] = interfaces;
        gateways[j_links[i]["source"].asString()] = interfaces.GetAddress(0);
        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    }

    /*
     * Configure the static default routes for the hosts. The default routes
     * correspond to the next hop that they should send the traffic to. In our
     * case the router they are attached to.
     */
    for (auto it = gateways.begin(); it != gateways.end(); it++)
    {
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Your code goes here
        Ptr<Ipv4StaticRouting> staticRouting = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting>(
            node_map[it->first]->GetObject<Ipv4>()->GetRoutingProtocol());
        staticRouting->SetDefaultRoute(it->second, 1);
        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    }

    /*
     * Iterate over the flows in the JSON config and create the
     * applications correspondingly. The applications consist of a
     * traffic source (on-off application) and a sink.
     *
     * Tip: Be careful in which situations you use the source and in which
     *      situations you use the destination node.
     */
    NS_LOG_INFO("Create Applications");
    for (int i = 0; i < (int)j_flows.size(); i++)
    {
        int port = j_flows[i]["DstPort"].asInt();
        std::string u = j_flows[i]["Source"].asString();
        std::string v = j_flows[i]["Sink"].asString();
        std::stringstream key;
        key << u << v;

        if (j_flows[i]["Type"].asString() == "OnOff")
        {
            //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Your code goes here
            OnOffHelper onOff("ns3::UdpSocketFactory",
                              InetSocketAddress(out_iface[v].GetAddress(0), port));
            onOff.SetAttribute("DataRate", StringValue(j_flows[i]["DataRate"].asString()));
            onOff.SetAttribute("PacketSize", UintegerValue(1024));
            ApplicationContainer app = onOff.Install(node_map[u]);
            app.Start(Seconds(j_flows[i]["StartTime"].asDouble()));
            app.Stop(Seconds(j_flows[i]["StopTime"].asDouble()));

            PacketSinkHelper sink("ns3::UdpSocketFactory",
                                  InetSocketAddress(Ipv4Address::GetAny(), port));
            ApplicationContainer sinkApp = sink.Install(node_map[v]);
            sinkApp.Start(Seconds(j_flows[i]["StartTime"].asDouble()));
            sinkApp.Stop(Seconds(j_flows[i]["StopTime"].asDouble()));
            //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        }
        if (j_flows[i]["Type"].asString() == "Ping")
        {
            //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Your code goes here
            PingHelper ping(out_iface[v].GetAddress(0));
            ping.SetAttribute("StartTime", TimeValue(Seconds(j_flows[i]["StartTime"].asDouble())));
            ping.SetAttribute("StopTime", TimeValue(Seconds(j_flows[i]["StopTime"].asDouble())));
            ApplicationContainer app = ping.Install(node_map[u]);

            AsciiTraceHelper asciiTraceHelper;
            std::stringstream fname_rtt;
            fname_rtt << "output/rip/" << key.str() << ".rtt";
            Ptr<OutputStreamWrapper> streamRtt = asciiTraceHelper.CreateFileStream(fname_rtt.str());
            app.Get(0)->TraceConnectWithoutContext("Rtt", MakeBoundCallback(&TraceRtt, streamRtt));
            //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        }
    }

    /*
     * Schedule the link failure events.
     */
    for (int i = 0; i < (int)j_fails.size(); i++)
    {
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Your code goes here
        std::string u = j_fails[i]["source"].asString();
        std::string v = j_fails[i]["target"].asString();
        double startTime = j_fails[i]["StartTime"].asDouble();
        double stopTime = j_fails[i]["StopTime"].asDouble();

        Simulator::Schedule(Seconds(startTime),
                            &Ipv4::SetDown,
                            node_map[u]->GetObject<Ipv4>(),
                            iface_nums[u][v]);
        Simulator::Schedule(Seconds(startTime),
                            &Ipv4::SetDown,
                            node_map[v]->GetObject<Ipv4>(),
                            iface_nums[v][u]);
        Simulator::Schedule(Seconds(stopTime),
                            &Ipv4::SetUp,
                            node_map[u]->GetObject<Ipv4>(),
                            iface_nums[u][v]);
        Simulator::Schedule(Seconds(stopTime),
                            &Ipv4::SetUp,
                            node_map[v]->GetObject<Ipv4>(),
                            iface_nums[v][u]);
        Simulator::Schedule(Seconds(startTime + 0.000001),
                            &Ipv4GlobalRoutingHelper::RecomputeRoutingTables);
        Simulator::Schedule(Seconds(stopTime + 0.000001),
                            &Ipv4GlobalRoutingHelper::RecomputeRoutingTables);
        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    }

    /*
     * Create a flow monitor and install it on all sources.
     */
    FlowMonitorHelper flowmonHelper;
    flowmonHelper.InstallAll();

    /*
     * Configure printing of the routing table. The tables should
     * be printed every second. The table of each node should be
     * written to a *separate* file.
     */
    RipHelper routingHelper;
    AsciiTraceHelper asciiTraceHelper;
    for (auto it = node_map.begin(); it != node_map.end(); it++)
    {
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Your code goes here
        std::stringstream fname;
        fname << "output/rip/" << it->first << "-routing.table";
        Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream(fname.str());
        routingHelper.PrintRoutingTableEvery(Seconds(1), it->second, stream);
        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    }

    NS_LOG_INFO("Run Simulation.");
    Simulator::Stop(Seconds(100));
    Simulator::Run();
    NS_LOG_INFO("Done.");

    /*
     * Configure flow monitor to write output to xml file
     */
    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Your code goes here
    flowmonHelper.SerializeToXmlFile("output/rip/flow-monitor-output.xml", true, true);
    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

    Simulator::Destroy();
    return 0;
}

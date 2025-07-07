//
// Created by developer on 6/22/20.
//
// #include "/home/developer/jsoncpp/include/json/json.h"
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
TraceRtt(std::ostream* os, Time rtt)
{
    *os << Simulator::Now().GetSeconds() << "\t" << rtt.GetMilliSeconds() << " ms" << std::endl;
}

int
main(int argc, char* argv[])
{
    // Set up some default values for the simulation.  Use the
    Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(210));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue("448kb/s"));
    // bool enableFlowMonitor = true;

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
     * Create the nodes in the network.
     */
    NS_LOG_INFO("Create nodes.");
    NodeContainer c;
    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Your code goes here
    std::cout << "Create " << j_nodes.size() << " nodes." << std::endl;
    c.Create(j_nodes.size());
    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

    /*
     * Create a mapping of node ids to nodes in the node container.
     * The name of the node in the JSON file has to map to a node created in the node
     * container. The nodes in the node container do not yet have any semantics to
     * them, yet. They are just plain nodes objects. The semantics (that is, links, applications,
     * etc.) are added to the nodes during the rest of this example.
     */
    std::unordered_map<std::string, Ptr<Node>> node_map;
    for (int i = 0; i < (int)j_nodes.size(); i++)
    {
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Your code goes here
        std::cout << "Create node " << j_nodes[i]["id"].asString() << std::endl;
        node_map[j_nodes[i]["id"].asString()] = c.Get(i);
        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    }

    /*
     * Use the previously created mapping to create node containers that represent
     * the links in the network topology. Use the identifiers of the nodes to
     * create a corresponding identifier for the edges.
     */
    std::unordered_map<std::string, NodeContainer> edges;
    for (int i = 0; i < (int)j_links.size(); i++)
    {
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Your code goes here
        std::cout << "Create link between " << j_links[i]["source"].asString() << " and "
                  << j_links[i]["target"].asString() << std::endl;
        std::string u = j_links[i]["source"].asString();
        std::string v = j_links[i]["target"].asString();
        NodeContainer link;
        link.Add(node_map[u]);
        link.Add(node_map[v]);
        edges[u + v] = link;
        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    }

    /*
     * Install the internet stack on all nodes.
     */
    InternetStackHelper internet;
    internet.Install(c);

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
        std::cout << "Create device for link " << key.str() << std::endl;
        p2p.SetDeviceAttribute("DataRate", StringValue(j_links[i]["DataRate"].asString()));
        p2p.SetChannelAttribute("Delay", StringValue(j_links[i]["Delay"].asString()));
        device_map[key.str()] = p2p.Install(edges[key.str()]);
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
    for (int i = 0; i < (int)j_links.size(); i++)
    {
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Your code goes here
        std::string source = j_links[i]["source"].asString();
        std::string target = j_links[i]["target"].asString();
        std::string network = j_links[i]["Network"].asString();

        if (node_map[target]->GetObject<Ipv4>()->GetNInterfaces() == 1)
        {
            std::swap(source, target);
        }

        std::cout << "Assign IP address to link between " << source << " and " << target
                  << " with network " << network << std::endl;

        ipv4.SetBase(network.c_str(), "255.255.255.0");
        out_iface[source + target] = ipv4.Assign(device_map[source + target]);
        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    }

    /*
     * Populate the routing tables.
     */
    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Your code goes here
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

    /*
     * Iterate over the flows in the JSON config and create the
     * applications correspondingly. The applications consist of a
     * traffic source (on-off application) and a sink.
     *
     * Tip: Be careful in which situations you use the source and in which
     *      situations you use the destination node.
     */
    std::cout << "configure applications" << std::endl;
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
            std::cout << "Create OnOff application from " << u << " to " << v << std::endl;
            int i = 0;
            for (auto test : out_iface)
            {
                std::cout << "Object: " << test.str() << std::endl;
            }

            std::cout << "interface: " << out_iface[key.str()].GetAddress(1) << std::endl;

            if (node_map.find(u) != node_map.end() && node_map.find(v) != node_map.end() &&
                out_iface.find(key.str()) != out_iface.end())
            {
                OnOffHelper onOff("ns3::UdpSocketFactory",
                                  InetSocketAddress(out_iface[key.str()].GetAddress(1), port));
                onOff.SetConstantRate(DataRate(j_flows[i]["DataRate"].asString()));
                ApplicationContainer app = onOff.Install(node_map[u]);
                app.Start(Seconds(j_flows[i]["StartTime"].asDouble()));
                app.Stop(Seconds(j_flows[i]["StopTime"].asDouble()));

                PacketSinkHelper sink("ns3::UdpSocketFactory",
                                      InetSocketAddress(Ipv4Address::GetAny(), port));
                ApplicationContainer sinkApp = sink.Install(node_map[v]);
                sinkApp.Start(Seconds(j_flows[i]["StartTime"].asDouble()));
                sinkApp.Stop(Seconds(j_flows[i]["StopTime"].asDouble()));
            }
            //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        }
        if (j_flows[i]["Type"].asString() == "Ping")
        {
            //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Your code goes here
            std::cout << "Create Ping application from " << u << " to " << v << std::endl;

            PingHelper ping(out_iface[key.str()].GetAddress(1),
                            node_map[u]->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal());
            ping.SetAttribute("StartTime", TimeValue(Seconds(j_flows[i]["StartTime"].asDouble())));
            ping.SetAttribute("StopTime", TimeValue(Seconds(j_flows[i]["StopTime"].asDouble())));
            ApplicationContainer app = ping.Install(node_map[u]);

            // Trace RTT

            // Set up RTT tracing
            std::stringstream fname_rtt;
            fname_rtt << "output/dgr/single_tcp_" << j_flows[i]["Name"].asString() << ".rtt";
            AsciiTraceHelper asciiTraceHelper;
            Ptr<OutputStreamWrapper> streamRtt = asciiTraceHelper.CreateFileStream(fname_rtt.str());

            // Connect the trace source for RTT to the TraceRtt function
            app.Get(0)->TraceConnectWithoutContext(
                "Rtt",
                MakeBoundCallback(&TraceRtt, streamRtt->GetStream()));
        }
        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    }

    /*
     * Schedule the events.
     */
    for (int i = 0; i < (int)j_fails.size(); i++)
    {
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Your code goes here
        std::string u = j_fails[i]["source"].asString();
        std::string v = j_fails[i]["target"].asString();
        std::stringstream key;
        key << u << v;
        Simulator::Schedule(Seconds(j_fails[i]["Time"].asDouble()),
                            &NetDevice::SetAttribute,
                            device_map[key.str()].Get(0),
                            "ReceiveErrorModel",
                            PointerValue(CreateObject<RateErrorModel>()));

        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    }

    /*
     * Create a flow monitor and install it on all sources.
     */
    FlowMonitorHelper flowmonHelper;
    flowmonHelper.InstallAll();

    NS_LOG_INFO("Run Simulation.");
    Simulator::Stop(Seconds(100));
    Simulator::Run();
    NS_LOG_INFO("Done.");

    /*
     * Configure flow monitor to write output to xml file
     */
    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Your code goes here
    flowmonHelper.SerializeToXmlFile("dynamic_global_routing_flowmon.xml", true, true);
    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

    Simulator::Destroy();
    return 0;
}

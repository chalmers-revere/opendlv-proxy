/*
 * Copyright (C) 2018 Ola Benderius
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <chrono>
#include <iostream>
#include <thread>

#include "cluon-complete.hpp"

int32_t main(int32_t argc, char **argv)
{
  int32_t retCode{0};
  auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
  if (0 == commandlineArguments.count("cid") || 0 == commandlineArguments.count("target-ip") || 0 == commandlineArguments.count("target-port") || 0 == commandlineArguments.count("port")) {
    std::cerr << argv[0] << " is a proxy to relay messages to and from an OpenDLV session." << std::endl;
    std::cerr << "Usage:   " << argv[0] << " --cid=<OpenDaVINCI session> --target-ip=<IP of the target opendlv-proxy microservice> --target-port=<UDP port of the target opendlv-proxy microservice> --port=<UDP port to listen on for messages being sent to this microservice> [--sender-stamp-offset=<The offset added to the sender stamp of each received message before being injected into the conference>] [--sender-stamp-max=<The largest expected sender stamp in the target conference, used when determining if a message should be relayed. Default: 5>] [--verbose]" << std::endl;
    std::cerr << "Example: " << argv[0] << " --cid=111 --target-ip=192.168.0.1 --target-port=10001 --port=10000 --sender-stamp-offset=1000" << std::endl;
    retCode = 1;
  } else {
    bool const VERBOSE{commandlineArguments.count("verbose") != 0};
    uint16_t const CID = static_cast<uint16_t>(std::stoi(commandlineArguments["cid"]));
    std::string const TARGET_IP = commandlineArguments["target-ip"];
    uint16_t const TARGET_PORT = static_cast<uint16_t>(std::stoi(commandlineArguments["target-port"]));
    uint16_t const PORT = static_cast<uint16_t>(std::stoi(commandlineArguments["port"]));

    uint32_t const SENDER_STAMP_OFFSET{(commandlineArguments["sender-stamp-offset"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["sender-stamp-offset"])) : 0};
    uint32_t const SENDER_STAMP_MAX{(commandlineArguments["sender-stamp-max"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["sender-stamp-max"])) : 5};

    cluon::UDPSender sender{TARGET_IP, TARGET_PORT};

    auto onDataFromOd4Session{[&VERBOSE, &SENDER_STAMP_OFFSET, &SENDER_STAMP_MAX, &sender](cluon::data::Envelope &&envelope)
      {
        uint32_t const DATA_TYPE = envelope.dataType();
        uint32_t const SENDER_STAMP = envelope.senderStamp();
      
        int32_t const RESTORED_SENDER_STAMP = SENDER_STAMP - SENDER_STAMP_OFFSET;
        if (RESTORED_SENDER_STAMP < 0 || RESTORED_SENDER_STAMP > static_cast<int32_t>(SENDER_STAMP_MAX)) {
          if (VERBOSE) {
            std::cout << "Message " << DATA_TYPE << " with sender stamp " << RESTORED_SENDER_STAMP << " was discarded." << std::endl;
          }
          return;
        }

        envelope.senderStamp(RESTORED_SENDER_STAMP);

        if (VERBOSE) {
          std::cout << "Sending message " << DATA_TYPE << " with sender stamp " << RESTORED_SENDER_STAMP << "." << std::endl;
        }

        std::string data = cluon::serializeEnvelope(std::move(envelope));
        sender.send(std::move(data));
      }};
    
    cluon::OD4Session od4{CID, onDataFromOd4Session};

    auto onDataFromUdpTunnel{[&VERBOSE, &CID, &SENDER_STAMP_OFFSET, &TARGET_IP, &od4](std::string &&data, std::string &&, std::chrono::system_clock::time_point &&) noexcept
      {
        std::stringstream sstr(data);
        while (sstr.good()) {
          auto tmp{cluon::extractEnvelope(sstr)};
          if (tmp.first) {
            cluon::data::Envelope envelope{tmp.second};

            uint32_t const DATA_TYPE = envelope.dataType();
            uint32_t const SENDER_STAMP = envelope.senderStamp();
            uint32_t const NEW_SENDER_STAMP = SENDER_STAMP + SENDER_STAMP_OFFSET;

            envelope.senderStamp(NEW_SENDER_STAMP);

            if (VERBOSE) {
              std::cout << "Injecting message " << DATA_TYPE << " with sender stamp " << NEW_SENDER_STAMP << " (was " << SENDER_STAMP << ") into conference " << CID << "." << std::endl;
            }

            envelope.sent(cluon::time::now());
            envelope.sampleTimeStamp(cluon::time::now());
            od4.send(std::move(envelope));
          }
        }
      }};

    cluon::UDPReceiver receiver{"0.0.0.0", PORT, onDataFromUdpTunnel};

    while (od4.isRunning() || receiver.isRunning()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
  return retCode;
}

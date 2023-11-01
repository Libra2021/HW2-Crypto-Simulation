#include "server.h"
#include "client.h"
#include "crypto.h"

std::vector<std::string> pending_trxs;

Server::Server() {}

std::shared_ptr<Client> Server::add_client(std::string id) {
    // find the same id
    if (get_client(id) != nullptr) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> distribution(0, 9);
        for (int i = 0; i < 4; i++) {
            id += std::to_string(distribution(gen));
        }
    }

    std::shared_ptr<Client> client_ptr = std::make_shared<Client>(id, *this);
    clients[client_ptr] = 5;
    return client_ptr;
}

std::shared_ptr<Client> Server::get_client(std::string id) const {
    for (const auto& clientEntry : clients) {
        std::shared_ptr<Client> client_ptr = clientEntry.first;
        if (client_ptr->get_id() == id) {
            return client_ptr;
        }
    }

    return nullptr;
}

double Server::get_wallet(std::string id) const {
    auto client_ptr = get_client(id);
    if (client_ptr) {
        // use const_iterator to get the value map to keep const-correctness
        auto it = clients.find(client_ptr);
        if (it != clients.end()) {
            return it->second;
        }
    }

    // default
    return 0;
}

bool Server::parse_trx(std::string trx, std::string& sender, std::string& receiver, double& value) {
    std::string regexPattern = "(\\w+)-(\\w+)-(\\d+.\\d+)";
    std::regex pattern(regexPattern);
    std::smatch matches;

    if (std::regex_search(trx, matches, pattern)) {
        sender = matches[1];
        receiver = matches[2];
        value = std::stod(matches[3]);
        return true;
    } else {
        throw std::runtime_error("Invalid trx!");
    }
}

bool Server::add_pending_trx(std::string trx, std::string signature) const {
    std::string sender, receiver;
    double value;

    Server::parse_trx(trx, sender, receiver, value);
    auto sender_ptr = get_client(sender);
    bool authentic = crypto::verifySignature(sender_ptr->get_publickey(), trx, signature);
    bool rich = sender_ptr->get_wallet() >= value;

    if (authentic && rich) {
        pending_trxs.push_back(trx);
        return true;
    }
    return false;
}

size_t Server::mine() {
    std::string mempool;
    for (auto& trx : pending_trxs) {
        mempool += trx;
    }

    // mine until success
    while (true) {
        for (const auto& clientEntry : clients) {
            std::shared_ptr<Client> client_ptr = clientEntry.first;
            size_t nonce = client_ptr->generate_nonce();
            std::string final_string = mempool + std::to_string(nonce);
            std::string hash{crypto::sha256(final_string)};

            std::regex pattern(".*0000.*");
            bool success = std::regex_match(hash.begin(), hash.begin() + 10, pattern);
            if (success) {
                // awarded
                clients[client_ptr] += 6.25;
                // execute pending_trxs
                for (auto& trx : pending_trxs) {
                    std::string sender, receiver;
                    double value;
                    parse_trx(trx, sender, receiver, value);
                    clients[get_client(sender)] -= value;
                    clients[get_client(receiver)] += value;
                }
                // clear pending_trxs vector
                pending_trxs.clear();
                // print the successful miner
                std::cout << "successful miner: " << client_ptr->get_id() << std::endl;

                return nonce;
            }
        }
    }
}

void show_wallets(const Server& server) {
    std::cout << std::string(20, '*') << std::endl;
 	for(const auto& client: server.clients) {
 		std::cout << client.first->get_id() <<  " : "  << client.second << std::endl;
    }
 	std::cout << std::string(20, '*') << std::endl;
}
#include "client.h"
#include "server.h"
#include "crypto.h"

Client::Client(std::string id, const Server& server) : id(id), server(&server) {
    crypto::generate_key(public_key, private_key);
}

std::string Client::get_id() const {
    return id;
}

std::string Client::get_publickey() const {
    return public_key;
}

double Client::get_wallet() const {
    return server->get_wallet(id);
}

std::string Client::sign(std::string txt) const {
    return crypto::signMessage(private_key, txt);
}

bool Client::transfer_money(std::string receiver, double value) const {
    if (server->get_client(receiver) == nullptr) {
        return false;
    }

    std::string trx = id + '-' + receiver + '-' + std::to_string(value);
    std::string signature = sign(trx);
    return server->add_pending_trx(trx, signature);
}

size_t Client::generate_nonce() {
    std::random_device rd;
    std::default_random_engine generator(rd());
    std::uniform_int_distribution<size_t> distribution(0, std::numeric_limits<size_t>::max());
    return distribution(generator);
}
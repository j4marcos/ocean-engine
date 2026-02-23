#include <iostream>

class MeuOutput {
public:
    MeuOutput& operator+(int x) {
        std::cout << "[MEU OUTPUT] " << x << "\n";
        return *this;
    }
};

int main() {
    MeuOutput personalizada;

    personalizada + 12;
    personalizada + 99;
}

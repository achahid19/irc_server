#include <iostream>
#include <cstdlib> // atoi
#include <sys/socket.h> // socket sys call
#include <netinet/in.h> // sockaddr_in structure
#include <arpa/inet.h> // inet_addr function for host
#include <sys/epoll.h> // epoll functions
#include <unistd.h> // close() function
#include <exception>
#include <cstring> // strerror
#include <cerrno> // errno for error handling
#include <signal.h> // signal handling

#include <map>
#include <vector>
#include <set>

void fy(){
    throw std::runtime_error("lw alo\n");
}

void fucion (){
    fy();

}

int main(){
    while (true){
            try
            {
                fucion();
        std::string buf;
        std::cout << "entre: \n";
        getline(std::cin, buf);
        if (std::isdigit(atoi(buf.c_str())) == false)
            throw std::runtime_error("not a digt\n");
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }
    }
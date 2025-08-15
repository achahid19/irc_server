#include "Channel.hpp"
#include "User.hpp"

void    errorHandling1(void){

}


int main(){
    std::map<int, void(*)(void)> erros;
    erros[452]= errorHandling1;
}

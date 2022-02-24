#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>

using namespace std;

//definiçes de tipos
typedef unsigned char byte;
typedef unsigned int  word;
typedef unsigned long microcode;

//estrutura para guardar uma microinstruçao decodificada
struct decoded_microcode
{
    word nadd;
    byte jam;
    byte sft;
    byte alu;
    word reg_w;
    byte mem;
    byte reg_r;
};

//Funções utilitárias ======================
void write_microcode(microcode w) //Dado uma microinstrucao, exibe na tela devidamente espaçado pelas suas partes.
{
   unsigned int v[36];
   for(int i = 35; i >= 0; i--)
   {
       v[i] = (w & 1);
       w = w >> 1;
   }

   for(int i = 0; i < 36; i++)
   {
       cout << v[i];
       if(i == 8 || i == 11 || i == 13 || i == 19 || i == 28 || i == 31) cout << " ";
   }
}

void write_word(word w) //Dada uma palavra (valor de 32 bits / 4 bytes), exibe o valor binário correspondente.
{
   unsigned int v[32];
   for(int i = 31; i >= 0; i--)
   {
       v[i] = (w & 1);
       w = w >> 1;
   }

   for(int i = 0; i < 32; i++)
       cout << v[i];
}

void write_byte(byte b) //Dado um byte (valor de 8 bits), exibe o valor binário correspondente na tela.
{
   unsigned int v[8];
   for(int i = 7; i >= 0; i--)
   {
       v[i] = (b & 1);
       b = b >> 1;
   }

   for(int i = 0; i < 8; i++)
       cout << v[i];
}

void write_dec(word d) //Dada uma palavra (valor de 32 bits / 4 bytes), exibe o valor decimal correspondente.
{
   cout << (int)d << endl;
}
//=========================================

//sinalizador para desligar máquina
bool halt = false;

//memoria principal
#define MEM_SIZE 0xFFFF+1 //0xFFFF + 0x1; // 64 KBytes = 64 x 1024 Bytes = 65536 (0xFFFF+1) x 1 Byte;
byte memory[MEM_SIZE]; //0x0000 a 0xFFFF (0 a 65535)

//registradores
word mar=0, mdr=0, pc=0, sp=0, lv=0, cpp=0, tos=0, opc=0, h=0;
byte mbr=0;

//barramentos
word bus_a=0, bus_b=0, bus_c=0, alu_out=0;

//estado da ALU para salto condicional
byte n=0, z=1;

//registradores de microprograma
word mpc;

//memória de microprograma: 512 x 64 bits = 512 x 8 bytes = 4096 bytes = 4 KBytes.
//Cada microinstrução é armazenada em 8 bytes (64 bits), mas apenas os 4,5 bytes (36 bits) de ordem mais baixa são de fato decodificados.
//Os 28 bits restantes em cada posição da memória são ignorados, mas podem ser utilizados para futuras melhorias nas microinstruções para controlar microarquiteturas mais complexas.
microcode microprog[512];

//carrega microprograma
//Escreve um microprograma de controle na memória de controle (array microprog, declarado logo acima)
void load_microprog()
{
  fstream rom;
    rom.open ("microprog.rom", ios::in | ios::out | ios::binary);

    for (int i = 0; i < 512; i++)
    {
      rom.read ((char *) &microprog[i], 8);
    }

    rom.close();

//carrega programa na memória principal para ser executado pelo emulador.
//programa escrito em linguagem de máquina (binário) direto na memória principal (array memory declarado mais acima).
}
void load_prog()
{
   FILE *f = fopen("prog.exe", "rb");
    word Q;
    fread(&Q, sizeof(word), 1, f); 
    fread(&memory[0], sizeof(byte), 20, f);
    word pc;
    memcpy(&pc, memory + 12, 4);
    fread(memory +pc +1, sizeof(byte), Q-20, f);
    fclose;

}

//exibe estado da máquina
void debug(bool clr = true)
{
    if(clr) system("clear");

    cout << "Microinstrução: ";
    write_microcode(microprog[mpc]);

    cout << "\n\nMemória principal: \nPilha: \n";
    for(int i = lv*4; i <= sp*4; i+=4)
    {
        write_byte(memory[i+3]);
        cout << " ";
        write_byte(memory[i+2]);
        cout << " ";
        write_byte(memory[i+1]);
        cout << " ";
        write_byte(memory[i]);
        cout << " : ";
        if(i < 10) cout << " ";
        cout << i << " | " << memory[i+3] << " " << memory[i+2] << " " << memory[i+1] << " " << memory[i];
        word w;
        memcpy(&w, &memory[i], 4);
        cout << " | " << i/4 << " : " << w << endl;
    }

    cout << "\n\nPC: \n";
    for(int i = (pc-1); i <= pc+20; i+=4)
    {
        write_byte(memory[i+3]);
        cout << " ";
        write_byte(memory[i+2]);
        cout << " ";
        write_byte(memory[i+1]);
        cout << " ";
        write_byte(memory[i]);
        cout << " : ";
        if(i < 10) cout << " ";
        cout << i << " | " << memory[i+3] << " " << memory[i+2] << " " << memory[i+1] << " " << memory[i];
        word w;
        memcpy(&w, &memory[i], 4);
        cout << " | " << i/4 << " : " << w << endl;
    }

    cout << "\nRegistradores - \nMAR: " << mar << " ("; write_word(mar);
    cout << ") \nMDR: " << mdr << " ("; write_word(mdr);
    cout << ") \nPC : " << pc << " ("; write_word(pc);
    cout << ") \nMBR: " << (int) mbr << " ("; write_byte(mbr);
    cout << ") \nSP : " << sp << " (";  write_word(sp);
    cout << ") \nLV : " << lv << " ("; write_word(lv);
    cout << ") \nCPP: " << cpp << " ("; write_word(cpp);
    cout << ") \nTOS: " << tos << " ("; write_word(tos);
    cout << ") \nOPC: " << opc << " ("; write_word(opc);
    cout << ") \nH  : " << h << " ("; write_word(h);
    cout << ")" << endl;
      
}

decoded_microcode decode_microcode(microcode code) //Recebe uma microinstrução binária e separa suas partes preenchendo uma estrutura de microinstrucao decodificada, retornando-a.
{
    decoded_microcode dec;
    microcode mascNadd = code >> 27;
    word newNadd = mascNadd;
    dec.nadd = newNadd;

    microcode mascJam = code >> 24;
    byte newJam = mascJam & 0b111;
    dec.jam = newJam;

    microcode mascSft = code >> 22;
    byte newSft = mascSft & 0b11;
    dec.sft = newSft;

    microcode mascUla = code >> 16;
    byte newUla = mascUla & 0b111111;
    dec.alu = newUla;

    microcode mascC = code >> 7;
    word newC = mascC & 0b111111111;
    dec.reg_w = newC;

    microcode mascMem = code >> 4;
    byte newMem = mascMem & 0b111;
    dec.mem = newMem;

    microcode mascB = code >> 0;
    byte newB = mascB & 0b1111;
    dec.reg_r = newB;

    return dec;
}

//alu
//recebe uma operação de alu binária representada em um byte (ignora-se os 2 bits de mais alta ordem, pois a operação é representada em 6 bits)
//e duas palavras (as duas entradas da alu), carregando no barramento alu_out o resultado da respectiva operação aplicada às duas palavras.
void alu(byte func, word a, word b)
{
    if(func == 0b00011000){
     alu_out = a;
    }
    else if(func == 0b00010100){
      alu_out = b;
    }
    else if(func == 0b00011010){
     
      alu_out = ~a;
    }
     else if(func == 0b00101100){
     
       alu_out = ~b;
     }
     else if(func == 0b00111100){
       alu_out = a + b;
     }
     else if(func == 0b00111101){
       alu_out = a + b + 1 ;
     }
     else if (func == 0b00111001){
       alu_out = a + 1 ;
     }
     else if (func == 0b00110101){
       alu_out = b + 1 ;
     }
     else if (func == 0b00111111){
       alu_out = b - a ;
     }
     else if (func == 0b00110110){
       alu_out = b - 1 ;
     }
     else if (func == 0b00111011){
       alu_out = -a ;
     }
     else if (func == 0b00001100){
       alu_out = a & b ;
     }
    else if (func == 0b00011100){
      alu_out = a | b ;
    }
    else if (func== 0b00010000){
      alu_out = 0 ;
     }
     else if (func== 0b00110001){
       alu_out = 1;
     }
     else if (func == 0b00110010){
       alu_out = -1 ;
     }

     if (alu_out) {
        n = 1;
        z = 0;
      } else {
          n = 0;
          z = 1;
      }
}

//Deslocamento. Recebe a instrução binária de deslocamento representada em um byte (ignora-se os 6 bits de mais alta ordem, pois o deslocador eh controlado por 2 bits apenas)
//e uma palavra (a entrada do deslocador) e coloca o resultado no barramento bus_c.
void shift(byte s, word w)
{
  //00 - Nenhum deslocamento
  //01 - Deslocamento de 8 bits à esquerda
  //10 - Deslocamento de 1 bit à direita
   
    if(s == 0b00000001){
      bus_c = w << 8;
    }else if(s == 0b00000010){
      bus_c = w >> 1;
    }
    else{
      bus_c = w;
    }
 
}

//Leitura de registradores. Recebe o número do registrador a ser lido (0 = mdr, 1 = pc, 2 = mbr, 3 = mbru, ..., 8 = opc) representado em um byte,
//carregando o barramento bus_b (entrada b da ALU) com o valor do respectivo registrador e o barramento bus_a (entrada A da ALU) com o valor do registrador h.
void read_registers(byte reg_end)
{

  if(reg_end == 0b000000000){
    bus_b = mdr;
    bus_a = h;

  }else if(reg_end == 0b00000001){
    bus_b = pc;
    bus_a = h;
   
  }else if(reg_end == 0b00000010){
    bus_b = mbr;
    word teste = mbr;
    byte primeiro_bit = mbr >> 7;
    if(primeiro_bit == 0b00000001){
      bus_b = teste | 0b1111111111111111111111100000000;
    }
    bus_a = h;
 
  }else if(reg_end == 0b00000011){
    bus_b = mbr;
    bus_a = h;
 
  }else if(reg_end == 0b00000100){
    bus_b = sp;
    bus_a = h;

  }else if(reg_end == 0b00000101){
     bus_b = lv;
     bus_a = h;

  }else if( reg_end == 0b00000110){
    bus_b = cpp;
    bus_a = h;

  }else if( reg_end == 0b00000111){
     bus_b = tos;
     bus_a = h;

  }else if( reg_end == 0b00001000){
     bus_b = opc;
     bus_a = h;
  }

}

//Escrita de registradores. Recebe uma palavra (valor de 32 bits) cujos 9 bits de ordem mais baixa indicam quais dos 9 registradores que
//podem ser escritos receberao o valor que está no barramento bus_c (saída do deslocador).
void write_register(word reg_end)
{
    if(reg_end  & 0b00000000000000000000000100000000){
      h = bus_c;
    }
    if(reg_end  & 0b00000000000000000000000010000000){
      opc = bus_c;
    }
    if(reg_end  & 0b00000000000000000000000001000000){
      tos = bus_c;
    }
    if(reg_end  & 0b00000000000000000000000000100000){
      cpp = bus_c;
    }
    if(reg_end  & 0b00000000000000000000000000010000){
      lv = bus_c;
    }
    if(reg_end  & 0b00000000000000000000000000001000){
      sp = bus_c;
    }
    if(reg_end  & 0b00000000000000000000000000000100){
      pc = bus_c;
    }
    if(reg_end  & 0b00000000000000000000000000000010){
      mdr = bus_c;
    }
    if(reg_end  & 0b00000000000000000000000000000001){
      mar = bus_c;
    }
}

//Leitura e escrita de memória. Recebe em um byte o comando de memória codificado nos 3 bits de mais baixa ordem (fetch, read e write, podendo executar qualquer conjunto dessas três operações ao
//mesmo tempo, sempre nessa ordem) e executa a respectiva operação na memória principal.
//fetch: lê um byte da memória principal no endereço constando em PC para o registrador MBR. Endereçamento por byte.
//write e read: escreve e lê uma PALAVRA na memória principal (ou seja, 4 bytes em sequência) no endereço constando em MAR com valor no registrador MDR. Nesse caso, o endereçamento é dado em palavras.
//Mas, como a memoria principal eh um array de bytes, deve-se fazer a conversão do endereçamento de palavra para byte (por exemplo, a palavra com endereço 4 corresponde aos bytes 16, 17, 18 e 19).
//Lembrando que esta é uma máquina "little endian", isto é, os bits menos significativos são os de posições mais baixas.
//No exemplo dado, suponha os bytes:
//16 = 00110011
//17 = 11100011
//18 = 10101010
//19 = 01010101
//Tais bytes correspondem à palavra 01010101 10101010 11100011 00110011
void mainmemory_io(byte control)
{
  //fetch
    if (control == 0b000000001){
       mbr =  memory[pc];
    }
  //read =  leitura
  if (control == 0b000000010){
      memcpy(&mdr, &memory[mar*4], 4);
    }
  // write = escrita
  if (control == 0b000000100){
      memcpy(&memory[mar*4],&mdr,4);
  }

}

//Define próxima microinstrução a ser executada. Recebe o endereço da próxima instrução a ser executada codificado em uma palavra (considera-se, portanto, apenas os 9 bits menos significativos)
//e um modificador (regra de salto) codificado em um byte (considera-se, portanto, apenas os 3 bits menos significativos, ou seja JAMZ (bit 0), JAMN (bit 1) e JMPC (bit 2)), construindo e
//retornando o endereço definitivo (codificado em uma word - desconsidera-se os 21 bits mais significativos (são zerados)) da próxima microinstrução.
word next_address(word next, byte jam)
{
 
    if(jam & 0b010){
      next = next | (n<< 8);
      return next;
    }
    if (jam & 0b001){
       next = next | (z<< 8);
       return next;
    }
    if(jam & 0b100){
      next = mbr | next;
      return next;
    }
   

    return next;
}

int main(int argc, char* argv[])
{
    load_microprog(); //carrega microprograma de controle

    load_prog(); //carrega programa na memória principal a ser executado pelo emulador. Já que não temos entrada e saída, jogamos o programa direto na memória ao executar o emulador.

    decoded_microcode decmcode;

    //laço principal de execução do emulador. Cada passo no laço corresponde a um pulso do clock.
    //o debug mostra o estado interno do processador, exibindo valores dos registradores e da memória principal.
    //o getchar serve para controlar a execução de cada pulso pelo clique de uma tecla no teclado, para podermos visualizar a execução passo a passo.
    //Substitua os comentários pelos devidos comandos (na ordem dos comentários) para ter a implementação do laço.
    while(!halt)
    {
        debug();

        //implementar! Pega uma microinstrução no armazenamento de controle no endereço determinado pelo registrador contador de microinstrução e decodifica (gera a estrutura de microinstrução, ou seja, separa suas partes). Cada parte é devidamente passada como parâmetro para as funções que vêm a seguir.
        //implementar! Lê registradores
        decmcode = decode_microcode(microprog[mpc]);
        read_registers(decmcode.reg_r);
        //implementar! Executa alu
        alu(decmcode.alu, bus_a,  bus_b);
        //implementar! Executa deslocamento
        shift( decmcode.sft, alu_out);
        //implementar! Escreve registradores
        write_register(decmcode.reg_w);
        //implementar! Manipula memória principal
        mainmemory_io(decmcode.mem);
        //implementar! Determina endereço da microinstrução (mpc) a ser executada no próximo pulso de clock
        mpc = next_address( decmcode.nadd, decmcode.jam);
       
      getchar();
    }

    debug(false);

    return 0;
}



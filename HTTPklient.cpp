// KlientTcp.cpp : Ten plik zawiera funkcję „main”. W nim rozpoczyna się i kończy wykonywanie programu.
//
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <string>
#include <iostream>
#include <windows.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>

#pragma comment(lib, "ws2_32.lib")



using namespace std;

class Wiadomosc
{
private:
	std::string imie;
	std::string akcja;
	std::string lancuch;
public:
	void Ustaw(std::string imie = "brak", std::string akcja = "UPLD") {
		this->imie = imie;
		this->akcja = akcja;
	}

	string komunikat() {
		lancuch = "GET /getStatus/login?USER=" + imie + "&ACTION=" + akcja + " HTTP/1.0\r\n\r\n";

		return lancuch;
	}
};
int init(int port) 
{
	WSADATA wsaData;
	// inicjalizacja żądanej wersja biblioteki WinSock
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		std::cout << "error " << WSAGetLastError() << std::endl;
		return -1;
	}

	int conSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);//zmienna do łączenia do klienta
	if (conSocket < 0) //błąd
	{
		perror("Error at socket:");
		WSACleanup();
		return -1;
	}
	struct sockaddr_in clientService;
	memset(&clientService, '\0', sizeof clientService);
	clientService.sin_family = AF_INET; //IP4
	inet_pton(AF_INET, "127.0.0.1", &(clientService.sin_addr));
	clientService.sin_port = htons(port);//port
	if (connect(conSocket, (struct sockaddr*)&clientService, sizeof(clientService)) == INVALID_SOCKET)//błąd połączenia
	{
		perror("Error at connect:");
		closesocket(conSocket);
		WSACleanup();
		return -1;
	}
	return conSocket;
}

int main(void)
{
	int conSocket=init(8080);
	if (conSocket == -1) //nie zwalniamy w tym miejscu poniewaz w poprezdniej funkcji wszedzie zwolnilismy na wypadek bledow wiec tu już bysmy zwalniali juz raz zwolnione zasoby
	{
		printf("blad w funkcji init \n");
		return 1;
	}
	FILE* file;
	errno_t err;
	string nazwa;
	if ((err = fopen_s(&file, "odpowiedz.txt", "wb")) != 0)
	{
		cout << "plik sie nie otwiera" << endl;
		closesocket(conSocket);
		WSACleanup();
		return 1;
	}
	int reading_size;
	int const buffer_size = 2048;//wielkośc bufora do odczytu z pliku
	char buffer[buffer_size];
	Wiadomosc napis;
	napis.Ustaw("dawid", "UPLOAD");
	char buf[512];//tablica char do odczytu z pliku
	int Sendsize;
	int odczytane = 0;
	do 
	{
		Sendsize = send(conSocket, napis.komunikat().c_str()+odczytane, napis.komunikat().size()-odczytane, 0);//wysylanie wiadmoscoi do serwera
		
		if (Sendsize < 0)
		{
			cout << "error with sending data" << endl;
			fclose(file);
			closesocket(conSocket);
			WSACleanup();//sprawdz tu jeszcze
			return 1;
		}
		else if (Sendsize == 0)
		{
			cout << "send zwrocil 0" << endl;
			break;
		}
		else if(Sendsize>0)
		{
			odczytane += Sendsize;
		}
	} while (odczytane!= (napis.komunikat()).size());//wyslanie wielkosci napisu  zabezpieczenie
	odczytane = 0;
	string token;
	string naglowek;
	int zapisane;
	int i;
	int j;
	int szukane=0;
	do
	{
		reading_size = recv(conSocket, buffer, 10, 0);//zaloz ze naglowek jest pierwszy oraz rozdziel naglowek i odpowiedz na dwa stringi
		if (reading_size > 0) //został tu usunięty zapis do pliku ponieważ nie był obligatoryjny a mógł zaciemnić działanie recv()
		{
			naglowek.append(buffer,0 ,reading_size);
		}
		else if (reading_size==0) 
		{
			break;
		}
		else 
		{
			std::cout << "error WSAStartup" << WSAGetLastError() << std::endl;
			fclose(file);
			closesocket(conSocket);
			WSACleanup();
			return 1;
		}
		
		//to zabezpieczneie dziala dlatego poniewaz od razu gdy reading size pobierze to zapisuje do pliku oraz cala wiadmosc do tokena czylijuz bufor jest niepotrzebny i zostaje nadpisywany 
	} while (reading_size > 0);//bedzie tak dddlugo pobieralo az pobirze ojedno wiecej czyli juz wiecej nie bedzie sie dalo pobrac

	szukane = naglowek.find("\r\n\r\n");
	token.append(naglowek.begin()+szukane+4, naglowek.end());
	naglowek.erase(naglowek.begin() + szukane, naglowek.end());
	cout << token << endl << endl;
	cout << naglowek << endl << endl;
	i = token.find("TOKEN <br>");
	j = token.find("<br></body>");
	if (i > 0) //jest to warunek na srpawdzenie obecnosci  tokena 
	{
		for (int a = i + 10; a < j;a++) //pobreanie tokena wraz z wypisem na terminalu
		{
			cout << token[a];
		}
		
		cout << endl;
	}
	else
	{
		printf(" odpowiedz jest niejednoznaczna\n");
	}
	//i = naglowek.find("Error code: ");//jest to warunek na srpawdzenie czy polaczeni sie nie udalo
	cout << "Kod bleadu jest nastepujacy:";
	i = naglowek.find(" ");
	
	j = naglowek.find("Error code: ");///szukamy najpierw "Error code:" ponieważ zauważyłem że błędy typu 400 nie można wyszukać poprzednim sposobem i to wyszukuje 400
	if(j>0)
	{
		for (int a = j + 12;a < j + 15;a++) 
		{
			cout << naglowek[a];
		}
	}
	else if (i > 0)
	{
		for (int a = i + 1; a < i + 4;a++)//wypiasnie kod bledu ten naglowek wyszukuje bledy typu 500oraz 200
		{
			cout << naglowek[a];
		}

	}
	
	cout << "\n";
	fclose(file);
	closesocket(conSocket);
	WSACleanup(); //na koniec programu zwalniamy interfejs
}

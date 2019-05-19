#include <stdio.h>
#include <Windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <vector>
#include <cstdlib>
#include <chrono>
#include <cstdio>
#include <ctime>

using namespace std;
using namespace std::chrono;
typedef int(_fastcall *MyProc1)(int, int, float*, float*, float*);
typedef int(_fastcall *MyProc2)(int, int, float*, float*, float*);


float *GetMatrix(string name)
{
	fstream plik;
	plik.open(name, ios::in);
	if (plik.good())
	{
		int row, column;
		plik >> row; plik >> column;
		float *t;
		t = new float [row*column];
		int pos = 0;
		for (int i = 0; i < row; i++)
		{
			for (int j = 0; j < column; j++)
			{
				plik >> t[pos]; 
				pos++;
			}

		}
		return t;
	}
	else
	{
		cout << "Zla nazwa pliku: " << name << endl;
		exit(0);
	}
}
int *GetRowAndColumn(string name)
{
	fstream plik;
	plik.open(name, ios::in);
	if (plik.good())
	{
		int *t;
		t = new int [2];	
		plik >> t[0]; plik >> t[1];
		return t;
		
	}
	else
	{
		cout << "Zle dane w pliku: " << name << endl;
		exit(0);
	}
}
bool compareFiles()
{
	ifstream f1, f2;
	f1.open("OutputCpp.txt"); f2.open("OutputAsm.txt");
	
	string s1, s2;
	while ((!f1.eof()) && (!f2.eof()))
	{
		getline(f1, s1); getline(f2, s2);
		if (s1 != s2)
			return false;
				
	}	
	f1.close();
	f2.close();
	return true;

}
int main(int argc, char *argv[])
{
	///////////////////////////////////////////////////////////////////////////////
	HINSTANCE dllHandle = NULL;
	HINSTANCE dllHandle2 = NULL;

	dllHandle = LoadLibrary(L"DLL_ASM.dll");
	dllHandle2 = LoadLibrary(L"DLL_C++.dll");
	MyProc1 procedura = (MyProc1)GetProcAddress(dllHandle, "MyProc1");
	MyProc2 procedura2 = (MyProc2)GetProcAddress(dllHandle2, "MyProc2");
	////////////////////////////////////////////////////////////////////////////////
	//Odczyt z linii polecen 
	////////////////////////////////////////////////////////////////////////////////
	string fileName1, fileName2;
	string path = "Dane testowe/"; //ustawienie sciezki do plikow wejsciowych


	unsigned concurentThreadsSupported = std::thread::hardware_concurrency(); //liczba rdzeni
	for (int i = 0; i < argc; i++)
	{
		if (string(argv[i]) == "-i" && i + 2 <= argc)
		{
			fileName1 = path+argv[i + 1];
			fileName2 = path+argv[i + 2];
		}
		if (string(argv[i]) == "-t" && i + 1 <= argc)
		{
			
			concurentThreadsSupported = stoi(argv[i + 1]); //konwersja do inta
			if (concurentThreadsSupported > 64 || concurentThreadsSupported <= 0) //zakres watkow <1,64>
			{
				concurentThreadsSupported = std::thread::hardware_concurrency();
			}
		}
	}
	cout <<"Liczba watkow: " << concurentThreadsSupported <<endl;
	//////////////////////////////////////////////////////////////////////////////////
	float *t1 = GetMatrix(fileName1);
	float *t2 = GetMatrix(fileName2);
	int *sizeMatrix1 = GetRowAndColumn(fileName1);
	int *sizeMatrix2 = GetRowAndColumn(fileName2);

	
	//sprawdzanie czy macierze maja taki sam rozmiar
	if (sizeMatrix1[0] != sizeMatrix2[0] || sizeMatrix1[1] != sizeMatrix2[1])
	{
		cout << "Rozne rozmiary macierzy" << endl;
		return EXIT_FAILURE;
	}
	int numberOfRows = sizeMatrix1[0];
	int numberOfColumns = sizeMatrix1[1];
	
	int pos = 0; //zmienna do poruszania sie po macierzach

	//przydzial danych do watku -> tylko ostatni watek dostaje liczbe niepodzielna przez 4
	int rowsPerThread, restPos, posPerFour;
	int *dataPerThreads = new int[concurentThreadsSupported]; //tablica podzialu danych na watki
	posPerFour = numberOfRows*numberOfColumns / 4;//czworki
	restPos = (numberOfRows*numberOfColumns) % 4; //reszta z dzielenia przez 4 -> 0,1,2,3
	int posPerFourRest = posPerFour % concurentThreadsSupported; //reszta z dzielenia czworek na watki -> 0..liczba watkow-1
	posPerFour /= concurentThreadsSupported; //cale czworki na watki 
	rowsPerThread = numberOfRows*numberOfColumns / concurentThreadsSupported;

	for (int i = 0; i < concurentThreadsSupported; i++)
	{
		dataPerThreads[i] = posPerFour;
		if (posPerFourRest != 0)
		{
			dataPerThreads[i] ++;
			posPerFourRest --;
		}
		dataPerThreads[i] *= 4;
		
	}
	if (restPos != 0)
	{
		dataPerThreads[concurentThreadsSupported - 1] += restPos;
	}

	//przydzial pamieci na macierz wynikowa w C++
	float *destMatrixCpp;
	destMatrixCpp = new float [numberOfRows * numberOfColumns];
	//przydzial pamieci na macierz wynikowa w ASM
	float *destMatrixAsm;
	destMatrixAsm = new float [numberOfRows * numberOfColumns + 3];
	
	
	//tablice watkow
	thread *threadsAsm = new thread [concurentThreadsSupported];
	thread *threadsCpp = new thread[concurentThreadsSupported];

	int startingPos = 0;
	
	// wlaczanie czasu
	auto beginCpp= std::chrono::steady_clock::now();

	for (int i = 0; i < concurentThreadsSupported; i++)
	{
		threadsCpp[i] = thread(procedura2, startingPos, dataPerThreads[i], ref(t1), ref(t2), ref(destMatrixCpp));
		startingPos += dataPerThreads[i];
	}
	
	for (int i = 0; i < concurentThreadsSupported; i++)
	{
		threadsCpp[i].join();
	}

	//wylaczenie czasu
	auto endCpp = chrono::steady_clock::now();
	//liczenie czasu
	cout << "Czas dla cpp: " << chrono::duration_cast<chrono::microseconds>(endCpp- beginCpp).count() << " microseconds" << endl;

	//analogicznie jak dla Cpp
	auto beginAsm = std::chrono::steady_clock::now();
	startingPos = 0;
	for (int i = 0; i < concurentThreadsSupported; i++)
	{
		threadsAsm[i] = thread(procedura, startingPos, dataPerThreads[i], ref(t1), ref(t2), ref(destMatrixAsm));
		startingPos += dataPerThreads[i];
	}

	for (int i = 0; i < concurentThreadsSupported; i++)
	{
		threadsAsm[i].join();
	}
	auto endAsm = chrono::steady_clock::now();
	//liczenie czasu
	cout << "Czas dla asm: " << chrono::duration_cast<chrono::microseconds>(endAsm - beginAsm).count() << " microseconds" << endl;
	
	//zapisanie macierzy z Cpp i z ASM wynikowej do pliku 
	fstream plikCpp("OutputCpp.txt", ios::out);	
	fstream plikAsm("OutputAsm.txt", ios::out);
	if (!plikCpp.good() || (!plikAsm.good()))
	{
		cout << "NIE MA PLIKU!" << endl;
		EXIT_FAILURE;
	}
	else
	{
		pos = 0;
		for (int i = 0; i < numberOfRows; i++)
		{
			for (int j = 0; j < numberOfColumns; j++)
			{
				plikCpp << destMatrixCpp[pos] << " ";
				plikAsm << destMatrixAsm[pos] << " ";
				pos++;
			}
			plikCpp << endl;
			plikAsm << endl;
		 }
		cout << "Wynik dla Cpp znajduje sie w pliku OutputCpp.txt" << endl;
		cout << "Wynik dla Asm znajduje sie w pliku OutputAsm.txt" << endl;
	}
	
	if (compareFiles())
	{
		cout << "Pliki sa rowne" << endl;
	}
	else
	{
		cout << "Uwaga, pliki nie sa rowne!!!" << endl;
	}
	
	delete[] destMatrixAsm;
	delete[] destMatrixCpp;
	delete[] t1;
	delete[] t2;
	delete[]threadsAsm;
	delete[]threadsCpp;
	plikAsm.close();
	plikCpp.close();

	return 0;

}

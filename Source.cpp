#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace std;

//definicje
#define PAGESIZE 4
#define DATAFILE "data"
#define DATAFILE2 "data2"
#define TESTFILE "test.txt"
#define INDEXFILE "index"
#define INDEXSIZE 20 
#define ALPHA 0.5




struct Index
{
	int key;
	int pageNumber;
};

struct Record
{
	int deleted = 0;
	int key;
	int size;
	double* numbers;
	int pointer;

	Record()
	{
		deleted = 0;
		key = -1;
		size = -1;
		numbers = new double[15];
		pointer = -1;
	}
	Record(int key, int size, double* numbers)
	{
		this->deleted = 0;
		this->key = key;
		this->size = size;
		this->numbers = numbers;
		this->pointer = -1;
	}
};
//zmienne globalne
int overflowCounter = 0;
int numberOfPages = 2;
int allReads[5] = { 0,0,0,0,0 }; //0-add 1-delete 2-update 3-reorganizacja 4-find
int timesRead[5] = { 0,0,0,0,0 }; //0-add 1-delete 2-update 3-reorganizacja 4-find
int timesReads = 0;
int overflowPages = 2;

//definicje funkcji
void initialize();
void readTestFile();
void add(int, int, double*);
void showFile();
void showRecords();
void reorganize();
void find(int);
void updateRecord(int, int, double*);
void deleteRecord(int);

int main()
{
	initialize();
	char command= 't';
	bool show = false;
	cout << "Czy chcesz aby program wczytal dane z pliku testowego?(t/n)\n";
	cin >> command;
	if (command == 't')
	{
		readTestFile();
		return 0;
	}
	command = 'n';
	cout << "Czy chcesz aby program wyswietlal po kazdej operacji u/d/a zawartosc pliku z danymi i indeksu?(t/n)\n";
	cin >> command;
	if (command == 't') show = true;
	while (command != 'e')
	{
		cout << "\nWybierz komende(e - exit, a - add, r - reorganize, f - find record, u - update, d - delete, s - show):  ";
		cin >> skipws >> command;
		if (command == 'a')
		{
			double* numbers = new double[15];
			char endlDetector = ' ';
			int key;
			cout << "\nwpisz klucz rekordu: ";
			cin >> key;
			cout << "\nwpisz kolejne liczby rekordu zakonczone znakiem konca linii: ";
			int counter = 0;
			while (cin >> numbers[counter] >> noskipws >> endlDetector)
			{
				counter++;
				if (endlDetector == '\n') break;
			}
			add(key, counter, numbers);
			if (show) showFile();
			
		}
		if (command == 'r') reorganize(); showFile();
		if (command == 's') showRecords();
		if (command == 'f')
		{
			int key;
			cout << "wpisz klucz poszukiwanego rekordu: ";
			cin >> key;
			find(key);
		}
		if (command == 'u')
		{
			double* numbers = new double[15];
			char endlDetector = ' ';
			int key;
			cout << "\nwpisz klucz rekordu: ";
			cin >> key;
			cout << "\nwpisz kolejne liczby rekordu zakonczone znakiem konca linii: ";
			int counter = 0;
			while (cin >> numbers[counter] >> noskipws >> endlDetector)
			{
				counter++;
				if (endlDetector == '\n') break;
			}
			updateRecord(key, counter, numbers);
			if (show) showFile();
		}
		if (command == 'd')
		{
			int key;
			cout << "wpisz klucz poszukiwanego rekordu: ";
			cin >> key;
			deleteRecord(key);
			if (show) showFile();
		}
	}
}



void readTestFile()
{
	ifstream file(TESTFILE);
	char command = ' ';
	while (file >> skipws >> command)
	{
		if (command == 'a')
		{
			double* numbers = new double[15];
			char endlDetector = ' ';
			int key;
			file >> key;
			int counter = 0;
			while (file >> numbers[counter] >> noskipws >> endlDetector)
			{
				counter++;
				if (endlDetector == '\n') break;
			}
			add(key, counter, numbers);
			//showFile();
		}
		if (command == 'r') reorganize();
		if (command == 's') showRecords();
		if (command == 'f')
		{
			
			int key;
			file >> skipws >> key;
			find(key);
		}
		if (command == 'u')
		{
			double* numbers = new double[15];
			char endlDetector = ' ';
			int key;
			file >> key;
			int counter = 0;
			while (file >> numbers[counter] >> noskipws >> endlDetector)
			{
				counter++;
				if (endlDetector == '\n') break;
			}
			updateRecord(key, counter, numbers);
			//showFile();
		}
		if (command == 'd')
		{
			int key;
			file >> skipws >> key;
			deleteRecord(key);
			//showFile();
		}
	};
	showFile();
	file.close();
}

void showFile()
{
	FILE* indexFile = fopen(INDEXFILE, "rb");
	Index* indexes = new Index[INDEXSIZE];
	cout << "Index:\n";
	for (int i = 0; i < numberOfPages; i++)
	{
		if (i % INDEXSIZE == 0)
		{
			fread(indexes, sizeof(Index), INDEXSIZE, indexFile);
		}
		cout << indexes[i % INDEXSIZE].key << " " << indexes[i % INDEXSIZE].pageNumber << "\n";
	}
	delete[] indexes;
	fclose(indexFile);

	cout << "\n\nPrimary area:\n";
	FILE* dataFile = fopen(DATAFILE, "rb");
	Record* records = new Record[PAGESIZE];
	for (int i=0; i<numberOfPages + overflowPages; i++)
	{
		if (i == numberOfPages)
		{
			cout << "\n\nOverflow area:\n";
		}
		cout << "Strona nr " << i % numberOfPages + 1<<"\n";
		fread(records, sizeof(Record), PAGESIZE, dataFile);
		for (int j = 0; j < PAGESIZE; j++)
		{
			if (records[j].key == -1)
			{
				cout << PAGESIZE - j << " wolnych miejsc\n\n";
				break;
			}
			cout << "usunieto: " << records[j].deleted;
			cout << " klucz: " << records[j].key;
			cout << " rekordy: ";
			for (int k = 0; k < records[j].size; k++) 
				cout << records[j].numbers[k] << " ";
			cout << "wskaznik: " << records[j].pointer << "\n";
		}
		cout << "\n";
	}
	delete[] records;
	fclose(dataFile);
}

void showRecords()
{
	FILE* indexFile = fopen(INDEXFILE, "rb");
	Index* indexes = new Index[INDEXSIZE];
	cout << "Index:\n";
	for (int i = 0; i < numberOfPages; i++)
	{
		if (i % INDEXSIZE == 0)
		{
			fread(indexes, sizeof(Index), INDEXSIZE, indexFile);
		}
		cout << indexes[i % INDEXSIZE].key << " " << indexes[i % INDEXSIZE].pageNumber << "\n";
	}
	delete[] indexes;
	fclose(indexFile);

	cout << "\n\nPrimary area:\n";
	FILE* dataFile = fopen(DATAFILE, "rb");
	Record* records = new Record[PAGESIZE];
	for (int i = 0; i < numberOfPages + overflowPages; i++)
	{
		if (i == numberOfPages)
		{
			cout << "\n\nOverflow area:\n";
		}
		//cout << "Strona nr " << i % numberOfPages + 1<<"\n";
		fread(records, sizeof(Record), PAGESIZE, dataFile);
		for (int j = 0; j < PAGESIZE; j++)
		{
			if (records[j].deleted) continue;
			if (records[j].key == -1)
			{
				//cout << PAGESIZE - j << " wolnych miejsc\n\n";
				break;
			}
			//cout << " rekordy: ";
			for (int k = 0; k < records[j].size; k++)
				cout << records[j].numbers[k] << " ";
			cout << "\n";
		}
	}
	delete[] records;
	fclose(dataFile);
}

void reorganize()
{
	int read = 0;
	FILE* indexFile = fopen(INDEXFILE, "wb");
	Index* indexes = new Index[INDEXSIZE];
	int actualOverflowPage = -1, currentAlpha = 0, pointer = -1, tmpNumberOfPages=0;
	Record *primaryRecords = new Record[PAGESIZE];
	Record *overflowRecords = new Record[PAGESIZE];
	Record *newFileRecords = new Record[PAGESIZE];
	FILE* readFile = fopen(DATAFILE, "rb");
	FILE* writeFile = fopen(DATAFILE2, "wb");

	for (int i = 0; i < numberOfPages; i++)
	{
		if (currentAlpha == ALPHA * PAGESIZE) // jesli zapelnilismy nowa strone to zapisujemy
		{
			fwrite(newFileRecords, sizeof(Record), PAGESIZE, writeFile);
			
			currentAlpha = 0;
		}
		
		fseek(readFile, i*sizeof(Record)*PAGESIZE, SEEK_SET);
		fread(primaryRecords, sizeof(Record), PAGESIZE, readFile);
		read++;
		for (int j = 0; j < PAGESIZE; j++)
		{

			if (currentAlpha == ALPHA * PAGESIZE) // jesli zapelnilismy nowa strone to zapisujemy
			{
				fwrite(newFileRecords, sizeof(Record), PAGESIZE, writeFile);
				
				currentAlpha = 0;
			}
			
			if (primaryRecords[j].key == -1) break; //jesli koniec strony
			if (primaryRecords[j].pointer == -1)
			{
				if (primaryRecords[j].deleted) continue;
				newFileRecords[currentAlpha] = primaryRecords[j];
				if (currentAlpha == 0) 
				{
					tmpNumberOfPages++;
					indexes[(tmpNumberOfPages - 1)%INDEXSIZE].key = primaryRecords[j].key;
					indexes[(tmpNumberOfPages - 1)%INDEXSIZE].pageNumber = tmpNumberOfPages-1;
					if (tmpNumberOfPages % INDEXSIZE == 0 && tmpNumberOfPages != 0) //jesli zapelnilismy indeks
					{
						fwrite(indexes, sizeof(Index), INDEXSIZE, indexFile);
					}
				}
				currentAlpha++;
				continue;
			}
			//jesli jest wskaznik
			pointer = primaryRecords[j].pointer;
			if (!primaryRecords[j].deleted)
			{
				newFileRecords[currentAlpha] = primaryRecords[j];
				if (currentAlpha == 0)
				{
					tmpNumberOfPages++;
					indexes[(tmpNumberOfPages - 1) % INDEXSIZE].key = primaryRecords[j].key;
					indexes[(tmpNumberOfPages - 1) % INDEXSIZE].pageNumber = tmpNumberOfPages - 1;
				}
				newFileRecords[currentAlpha].pointer = -1;
				currentAlpha++;
			}
			
			
			while (pointer != -1)
			{
				if (currentAlpha == ALPHA * PAGESIZE) // jesli zapelnilismy nowa strone to zapisujemy
				{
					fwrite(newFileRecords, sizeof(Record), PAGESIZE, writeFile);
					currentAlpha = 0;
				}
				
				if (actualOverflowPage != pointer / PAGESIZE) //jesli nie mamy w pamieci danej strony overflow
				{
					actualOverflowPage = pointer / PAGESIZE;
					fseek(readFile, (numberOfPages  + (int)(actualOverflowPage)) * sizeof(Record) * PAGESIZE, SEEK_SET);
					fread(overflowRecords, sizeof(Record), PAGESIZE, readFile);
					read++;
				}
				
				if (!overflowRecords[pointer % PAGESIZE].deleted)
				{
					newFileRecords[currentAlpha] = overflowRecords[pointer % PAGESIZE];
					if (currentAlpha == 0)
					{
						tmpNumberOfPages++;
						indexes[(tmpNumberOfPages - 1) % INDEXSIZE].key = overflowRecords[pointer % PAGESIZE].key;
						indexes[(tmpNumberOfPages - 1) % INDEXSIZE].pageNumber = tmpNumberOfPages - 1;
						if (tmpNumberOfPages % INDEXSIZE == 0 && tmpNumberOfPages != 0) //jesli zapelnilismy indeks
						{
							fwrite(indexes, sizeof(Index), INDEXSIZE, indexFile);
						}
					}
					newFileRecords[currentAlpha].pointer = -1;
					currentAlpha++;
				}
				pointer = overflowRecords[pointer % PAGESIZE].pointer;
				
			}
		}
	}
	if (currentAlpha != 0) //jesli cos nie zostalo zapisane
	{
		for (int i = ALPHA * PAGESIZE; i >= currentAlpha; i--)
		{
			newFileRecords[i].size = 0;
			newFileRecords[i].key = -1;
		}
		fwrite(newFileRecords, sizeof(Record), PAGESIZE, writeFile);
	}
	for (int i = 0; i < PAGESIZE; i++)
	{
		newFileRecords[i].key = -1;
		newFileRecords[i].size = 0;
	}
	overflowPages = numberOfPages / 5;
	for(int i=0;i<overflowPages;i++) fwrite(newFileRecords, sizeof(Record), PAGESIZE, writeFile);  //zapisanie overflow
	
	cout << "\nReorganizacja zuzyla " << read << " odczytow\n";
	timesRead[3]++;
	allReads[3]+=read;
	fwrite(indexes, sizeof(Index), INDEXSIZE, indexFile); //zapisanie indeksow
	numberOfPages = tmpNumberOfPages;
	overflowCounter = 0;
	delete[]primaryRecords;
	delete[]overflowRecords;
	delete[]newFileRecords;
	delete[]indexes;
	fclose(indexFile);
	fclose(readFile);
	fclose(writeFile);
	remove(DATAFILE);
	rename(DATAFILE2, DATAFILE);
}

void find(int key)
{
	int read = 0;
	FILE* indexFile = fopen(INDEXFILE, "rb");
	Index* indexes = new Index[INDEXSIZE];
	int page = numberOfPages - 1;
	for (int i = 0; i < numberOfPages; i++)
	{
		if (i % INDEXSIZE == 0)
		{
			read++;
			fread(indexes, sizeof(Index), INDEXSIZE, indexFile);
		}
		if (indexes[i % INDEXSIZE].key > key)
		{
			page = indexes[i % INDEXSIZE].pageNumber - 1;
			break;
		}
	}
	FILE* readFile = fopen(DATAFILE, "rb");
	Record* records = new Record[PAGESIZE];
	fseek(readFile, page * sizeof(Record) * PAGESIZE, SEEK_SET);
	fread(records, sizeof(Record), PAGESIZE, readFile);
	read++;
	int pointer = -1;
	for (int i = 0; i < PAGESIZE; i++)
	{
		if (records[i].key == key) //jesli znaleziono klucz
		{
			if (!records[i].deleted)
			{
				cout << "znaleziony rekord: ";
				for (int j = 0; j < records[i].size; j++)
					cout << records[i].numbers[j] << " ";
				cout << "\n";
				timesRead[4]++;
				allReads[4] += read;
			}
			else cout << "rekord o podanym kluczu zostal wczesniej usuniety\n";
			delete[] indexes;
			delete[] records;
			fclose(indexFile);
			fclose(readFile);
			return;
		}
		if (records[i].key > key)
		{
			pointer = records[i - 1].pointer;
			break;
		}
		if (i == PAGESIZE - 1)
		{
			pointer = records[i].pointer;
		}
	}
	if (pointer == -1)
	{
		cout << "Nie znaleziono rekordu o podanym kluczu\n";
		delete[] indexes;
		delete[] records;
		fclose(indexFile);
		fclose(readFile);
		return;
	}
	int actualOverflowPage = -1;
	while (pointer != -1)
	{
		if (actualOverflowPage != pointer / PAGESIZE) //jesli nie mamy w pamieci danej strony overflow
		{
			actualOverflowPage = pointer / PAGESIZE;
			fseek(readFile, (numberOfPages + (int)(actualOverflowPage)) * sizeof(Record) * PAGESIZE, SEEK_SET);
			fread(records, sizeof(Record), PAGESIZE, readFile);
			read++;
		}
		if (records[pointer % PAGESIZE].key == key)
		{
			if (!records[pointer % PAGESIZE].deleted)
			{
				cout << "znaleziony rekord: ";
				for (int j = 0; j < records[pointer % PAGESIZE].size; j++)
					cout << records[pointer % PAGESIZE].numbers[j] << " ";
				cout << "\n";
				timesRead[4]++;
				allReads[4] += read;
			}
			else cout << "rekord o podanym kluczu zostal wczesniej usuniety\n";
			delete[] indexes;
			delete[] records;
			fclose(indexFile);
			fclose(readFile);
			return;
		}
		if (records[pointer % PAGESIZE].pointer == -1)
		{
			cout << "Nie znaleziono rekordu o podanym kluczu\n";
			delete[] indexes;
			delete[] records;
			fclose(indexFile);
			fclose(readFile);
			return;
		}
		pointer = records[pointer % PAGESIZE].pointer;
	}
	cout << "Znalezienie klucza zuzylo " << read << " odczytow";
	timesRead[4]++;
	allReads[4]+=read;
	delete[] indexes;
	delete[] records;
	fclose(indexFile);
	fclose(readFile);
}

void deleteRecord(int key)
{
	int read = 0;
	int page = numberOfPages - 1;
	FILE* indexFile = fopen(INDEXFILE, "rb");
	Index* indexes = new Index[INDEXSIZE];
	for (int i = 0; i < numberOfPages; i++) //szukanie odpowiedniej strony
	{
		if (i % INDEXSIZE == 0)
		{
			fread(indexes, sizeof(Index), INDEXSIZE, indexFile);
			read++;
		}
		if (indexes[i % INDEXSIZE].key == -1)
		{
			page = indexes[i % INDEXSIZE].pageNumber;
			break;
		}
		if (indexes[i % INDEXSIZE].key > key)
		{
			page = indexes[i % INDEXSIZE].pageNumber - 1;
			break;
		}
	}

	delete[] indexes;
	fclose(indexFile);

	FILE* readFile = fopen(DATAFILE, "rb");
	FILE* writeFile = fopen(DATAFILE2, "wb");
	Record* records = new Record[PAGESIZE];
	int currentPage = 0;
	for (currentPage = 0; currentPage < page; currentPage++) //przepisanie zawartosci main area do tymczasowego pliku
	{
		fread(records, sizeof(Record), PAGESIZE, readFile);
		fwrite(records, sizeof(Record), PAGESIZE, writeFile);
	}
	fread(records, sizeof(Record), PAGESIZE, readFile);
	read++;

	int pointer = -2;
	for (int i = 0; i < PAGESIZE; i++) //szukanie rekordu o wiekszym kluczu
	{
		if (records[i].key == key) //jesli klucz jest na stronie
		{
			records[i].deleted = 1;
			pointer = -1;
			break;
		}
		if (records[i].key > key)
		{
			pointer = records[i - 1].pointer;
			if (pointer == -1)
			{
				cout << "Rekord o podanym kluczu nie istnieje\n";
				delete[] records;
				fclose(writeFile);
				fclose(readFile);
				remove(DATAFILE2);
				return;
			}
			break;
		}
	}
	if (pointer == -2)
	{
		pointer = records[PAGESIZE - 1].pointer;
		if (pointer == -1)
		{
			cout << "Rekord o podanym kluczu nie istnieje\n";
			delete[] records;
			fclose(writeFile);
			fclose(readFile);
			remove(DATAFILE2);
			return;
		}
	}
	fwrite(records, sizeof(Record), PAGESIZE, writeFile);
	for (currentPage = currentPage + 1; currentPage < numberOfPages; currentPage++) //przepisanie reszty do tymczasowego pliku
	{
		fread(records, sizeof(Record), PAGESIZE, readFile);
		fwrite(records, sizeof(Record), PAGESIZE, writeFile);
	}
	bool change = true;
	while (pointer != -1)
	{
		for (currentPage; currentPage < numberOfPages + (int)(pointer / PAGESIZE); currentPage++)
		{
			fread(records, sizeof(Record), PAGESIZE, readFile);
			fwrite(records, sizeof(Record), PAGESIZE, writeFile);
		}
		if (change)
		{
			fread(records, sizeof(Record), PAGESIZE, readFile);
			read++;
			change = false;
		}
		if (records[pointer % PAGESIZE].key == key) //jesli klucz istnieje
		{
			records[pointer % PAGESIZE].deleted = 1;
			fwrite(records, sizeof(Record), PAGESIZE, writeFile);
			break;
		}
		if (records[pointer % PAGESIZE].key > key || records[pointer % PAGESIZE].pointer == -1)
		{
			cout << "Rekord o podanym kluczu nie istnieje\n";
			delete[] records;
			fclose(writeFile);
			fclose(readFile);
			remove(DATAFILE2);
			return;
		}
		pointer = records[pointer % PAGESIZE].pointer;
		if (currentPage != numberOfPages + pointer / PAGESIZE)
		{
			fwrite(records, sizeof(Record), PAGESIZE, writeFile);
			currentPage++;
			change = true;
		}
	}
	for (currentPage; currentPage < numberOfPages + overflowPages; currentPage++) //przepisanie reszty do tymczasowego pliku
	{
		fread(records, sizeof(Record), PAGESIZE, readFile);
		fwrite(records, sizeof(Record), PAGESIZE, writeFile);
	}

	cout << "Usuniecie rekordu zuzylo " << read << " odczytow";
	timesRead[1]++;
	allReads[1] += read;
	fclose(writeFile);
	fclose(readFile);
	remove(DATAFILE);
	rename(DATAFILE2, DATAFILE);
	delete[] records;
}


void updateRecord(int key, int size, double* numbers)
{
	int page = numberOfPages - 1, read=0;
	FILE* indexFile = fopen(INDEXFILE, "rb");
	Index* indexes = new Index[INDEXSIZE];
	for (int i = 0; i < numberOfPages; i++) //szukanie odpowiedniej strony
	{
		if (i % INDEXSIZE == 0)
		{
			read++;
			fread(indexes, sizeof(Index), INDEXSIZE, indexFile);
		}
		if (indexes[i % INDEXSIZE].key == -1)
		{
			page = indexes[i % INDEXSIZE].pageNumber;
			break;
		}
		if (indexes[i % INDEXSIZE].key > key)
		{
			page = indexes[i % INDEXSIZE].pageNumber - 1;
			break;
		}
	}

	delete[] indexes;
	fclose(indexFile);

	FILE* readFile = fopen(DATAFILE, "rb");
	FILE* writeFile = fopen(DATAFILE2, "wb");
	Record* records = new Record[PAGESIZE];
	int currentPage = 0;
	for (currentPage = 0; currentPage < page; currentPage++) //przepisanie zawartosci main area do tymczasowego pliku
	{
		fread(records, sizeof(Record), PAGESIZE, readFile);
		fwrite(records, sizeof(Record), PAGESIZE, writeFile);
	}
	fread(records, sizeof(Record), PAGESIZE, readFile);
	read++;

	int pointer = -2;
	for (int i = 0; i < PAGESIZE; i++) //szukanie rekordu o wiekszym kluczu
	{
		if (records[i].key == key) //jesli klucz jest na stronie
		{
			records[i].size = size;
			records[i].numbers = numbers;
			records[i].deleted = 0;
			pointer = -1;
			break;
		}
		if (records[i].key > key)
		{
			pointer = records[i - 1].pointer;
			if (pointer == -1)
			{
				cout << "Rekord o podanym kluczu nie istnieje\n";
				delete[] records;
				fclose(writeFile);
				fclose(readFile);
				remove(DATAFILE2);
				return;
			}
			break;
		}
	}
	if (pointer == -2)
	{
		pointer = records[PAGESIZE - 1].pointer;
		if (pointer == -1)
		{
			cout << "Rekord o podanym kluczu nie istnieje\n";
			delete[] records;
			fclose(writeFile);
			fclose(readFile);
			remove(DATAFILE2);
			return;
		}
	}
	fwrite(records, sizeof(Record), PAGESIZE, writeFile);
	for (currentPage = currentPage + 1; currentPage < numberOfPages; currentPage++) //przepisanie reszty do tymczasowego pliku
	{
		fread(records, sizeof(Record), PAGESIZE, readFile);
		fwrite(records, sizeof(Record), PAGESIZE, writeFile);
	}
	bool change = true;
	while (pointer != -1) 
	{
		for (currentPage; currentPage < numberOfPages + (int)(pointer / PAGESIZE); currentPage++)
		{	
			fread(records, sizeof(Record), PAGESIZE, readFile);
			fwrite(records, sizeof(Record), PAGESIZE, writeFile);
		}
		if (change)
		{
			read++;
			fread(records, sizeof(Record), PAGESIZE, readFile);
			change = false;
		}
		if (records[pointer % PAGESIZE].key == key) //jesli klucz istnieje
		{
			records[pointer % PAGESIZE].size = size;
			records[pointer % PAGESIZE].numbers = numbers;
			records[pointer % PAGESIZE].deleted = 0;
			fwrite(records, sizeof(Record), PAGESIZE, writeFile);
			break;
		}
		if (records[pointer % PAGESIZE].key > key || records[pointer % PAGESIZE].pointer == -1)
		{
			cout << "Rekord o podanym kluczu nie istnieje\n";
			delete[] records;
			fclose(writeFile);
			fclose(readFile);
			remove(DATAFILE2);
			return;
		}
		pointer = records[pointer % PAGESIZE].pointer;
		if (currentPage != numberOfPages + pointer / PAGESIZE)
		{
			fwrite(records, sizeof(Record), PAGESIZE, writeFile);
			currentPage++;
			change = true;
		}
	}
	for (currentPage; currentPage < numberOfPages + overflowPages; currentPage++) //przepisanie reszty do tymczasowego pliku
	{
		fread(records, sizeof(Record), PAGESIZE, readFile);
		fwrite(records, sizeof(Record), PAGESIZE, writeFile);
	}

	cout << "Aktualizacja rekordu zuzyla " << read << " odczytow";
	timesRead[2]++;
	allReads[2] += read;
	fclose(writeFile);
	fclose(readFile);
	remove(DATAFILE);
	rename(DATAFILE2, DATAFILE);
	delete[] records;
}


void add(int key, int size, double*numbers)
{
	if (key < 0)
	{
		cout << "Nie mozna wstawic rekordu o kluczu mniejszym od 1\n";
		return;
	}
	if (overflowCounter == overflowPages * PAGESIZE) reorganize();  //jesli overflow jest zapelniony to reorganizujemy struktury
	int read = 0;
	int page = numberOfPages-1;
	FILE* indexFile = fopen(INDEXFILE, "rb");
	Index* indexes = new Index[INDEXSIZE];
	for (int i = 0; i < numberOfPages; i++) //szukanie odpowiedniej strony
	{
		if (i % INDEXSIZE == 0)
		{
			read++;
			fread(indexes, sizeof(Index), INDEXSIZE, indexFile);
		}
		if (indexes[i % INDEXSIZE].key == -1)
		{
			page = indexes[i % INDEXSIZE].pageNumber;
			break;
		}
		if (indexes[i % INDEXSIZE].key > key)
		{
			page = indexes[i % INDEXSIZE].pageNumber -1;
			break;
		}
	}
	delete[] indexes;
	fclose(indexFile);

	FILE* readFile = fopen(DATAFILE, "rb");
	FILE* writeFile = fopen(DATAFILE2, "wb");
	Record* records = new Record[PAGESIZE];
	int currentPage = 0;
	for (currentPage = 0; currentPage < page;currentPage++) //przepisanie zawartosci main area do tymczasowego pliku
	{
		fread(records, sizeof(Record), PAGESIZE, readFile);
		fwrite(records, sizeof(Record), PAGESIZE, writeFile);
	}
	read++;
	fread(records, sizeof(Record), PAGESIZE, readFile);
	if (records[PAGESIZE - 1].key == -1) //jesli mamy wolne miejsce na stronie
	{
		for (int i = PAGESIZE - 1; i > 0; i--)
		{
			if (records[i - 1].key < key && records[i-1].key !=-1)//wpisanie w odpowiednie miejsce
			{
				records[i].key = key;
				records[i].size = size;
				records[i].numbers = numbers;
				break;
			}
			if (records[i - 1].key == key) //jesli klucz juz istnieje
			{
				if (records[i - 1].deleted)
				{
					records[i - 1].key = key;
					records[i - 1].deleted = 0;
					records[i - 1].size = size;
					records[i - 1].numbers = numbers;
					break;
				}
				cout << "Rekord o podanym kluczu juz istnieje\n";
				
				delete[] records;
				fclose(writeFile);
				fclose(readFile);
				remove(DATAFILE2);
				return;
			}
			if (i == 1) 
			{
				records[i-1].key = key;
				records[i-1].size = size;
				records[i-1].numbers = numbers;
				break;
			}
			records[i].key = records[i - 1].key;
			records[i].size = records[i - 1].size;
			records[i].numbers = records[i - 1].numbers;
		}
		fwrite(records, sizeof(Record), PAGESIZE, writeFile);
		for (currentPage = currentPage+1; currentPage < numberOfPages + overflowPages; currentPage++) //przepisanie reszty do tymczasowego pliku
		{
			fread(records, sizeof(Record), PAGESIZE, readFile);
			fwrite(records, sizeof(Record), PAGESIZE, writeFile);
		}
	}
	else
	{
		int pointer = -2;
		for (int i = 0; i < PAGESIZE; i++) //szukanie rekordu o wiekszym kluczu
		{
			if (records[i].key > key)
			{
				pointer = records[i - 1].pointer;
				if (pointer == -1)
				{
					records[i - 1].pointer = overflowCounter;
					overflowCounter++;
				}
				break;
			}
			if (records[i].key == key) //jesli klucz juz istnieje
			{
				if (records[i].deleted)
				{
					records[i].key = key;
					records[i].deleted = 0;
					records[i].size = size;
					records[i].numbers = numbers;
					pointer = -4;
					break;
				}
				cout << "Rekord o podanym kluczu juz istnieje\n";
				delete[] records;
				fclose(writeFile);
				fclose(readFile);
				remove(DATAFILE2);
				return;
			}
		}
		if (pointer == -2)
		{
			pointer = records[PAGESIZE-1].pointer;
			if (pointer == -1)
			{
				records[PAGESIZE - 1].pointer = overflowCounter;
				overflowCounter++;
			}
		}
		fwrite(records, sizeof(Record), PAGESIZE, writeFile);
		for (currentPage = currentPage + 1; currentPage < numberOfPages; currentPage++) //przepisanie reszty do tymczasowego pliku
		{
			fread(records, sizeof(Record), PAGESIZE, readFile);
			fwrite(records, sizeof(Record), PAGESIZE, writeFile);
		}
		bool change = true;
		if (pointer == -1 || pointer == -4) fread(records, sizeof(Record), PAGESIZE, readFile); read++;
		while (pointer!=-1 && pointer!=-4) //szukanie ostatniego elementu listy
		{
			for (currentPage; currentPage < numberOfPages + (int)(pointer / PAGESIZE); currentPage++)
			{
				fread(records, sizeof(Record), PAGESIZE, readFile);
				fwrite(records, sizeof(Record), PAGESIZE, writeFile);
			}
			if (change)
			{
				fread(records, sizeof(Record), PAGESIZE, readFile);
				change = false;
				//read++;
			}
			if (records[pointer % PAGESIZE].key == key) //jesli klucz juz istnieje
			{
				if (records[pointer % PAGESIZE].deleted)
				{
					records[pointer % PAGESIZE].key = key;
					records[pointer % PAGESIZE].deleted = 0;
					records[pointer % PAGESIZE].size = size;
					records[pointer % PAGESIZE].numbers = numbers;
					pointer = -4;
					break;
				}
				cout << "Rekord o podanym kluczu juz istnieje\n";
				delete[] records;
				fclose(writeFile);
				fclose(readFile);
				remove(DATAFILE2);
				return;
			}
			if (records[pointer % PAGESIZE].key > key) //podmiana rekordu na mniejszy
			{
				swap(key, records[pointer % PAGESIZE].key);
				swap(size, records[pointer % PAGESIZE].size);
				swap(numbers, records[pointer % PAGESIZE].numbers);
			}
			if (records[pointer % PAGESIZE].pointer == -1) 
			{
				records[pointer % PAGESIZE].pointer = overflowCounter;
				overflowCounter++;
				pointer = -1;
				break;
			}
			pointer = records[pointer % PAGESIZE].pointer;
			if (currentPage != numberOfPages + pointer / PAGESIZE)
			{
				fwrite(records, sizeof(Record), PAGESIZE, writeFile);
				currentPage++;
				change = true;
			}
		}
		if (currentPage != numberOfPages + (int)((overflowCounter - 1) / PAGESIZE))
		{
			fwrite(records, sizeof(Record), PAGESIZE, writeFile); 
			currentPage++;
			for (currentPage; currentPage < numberOfPages + (int)((overflowCounter - 1) / PAGESIZE); currentPage++)
			{
				fread(records, sizeof(Record), PAGESIZE, readFile);
				fwrite(records, sizeof(Record), PAGESIZE, writeFile);
			}
			fread(records, sizeof(Record), PAGESIZE, readFile);
			read++;
		}
		if (pointer != -4)
		{
			records[(overflowCounter - 1) % PAGESIZE].key = key;
			records[(overflowCounter - 1) % PAGESIZE].numbers = numbers;
			records[(overflowCounter - 1) % PAGESIZE].size = size;
		}
		fwrite(records, sizeof(Record), PAGESIZE, writeFile);
		currentPage++;
		for (currentPage; currentPage < numberOfPages + overflowPages; currentPage++) //przepisanie reszty do tymczasowego pliku
		{
			fread(records, sizeof(Record), PAGESIZE, readFile);
			fwrite(records, sizeof(Record), PAGESIZE, writeFile);
		}
	}
	cout << "Dodanie rekordu zuzylo " << read << " odczytow\n";
	timesRead[0]++;
	allReads[0] += read;
	fclose(writeFile);
	fclose(readFile);
	remove(DATAFILE);
	rename(DATAFILE2, DATAFILE);
	delete[] records;
}

void initialize()
{

	Record* buffer = new Record[PAGESIZE];
	FILE* file = fopen(DATAFILE, "wb");
	for (int i = 0; i < numberOfPages + overflowPages; i++)
	{
		fwrite(buffer, sizeof(Record), PAGESIZE, file);
	}
	delete[]buffer;
	fclose(file);


	//inicjalizacja indeksow
	FILE* indexFile = fopen(INDEXFILE, "wb");
	Index* indexes = new Index[INDEXSIZE];
	for (int i = 0; i < numberOfPages; i++)
	{
		indexes[i%INDEXSIZE].key = -1;
		indexes[i%INDEXSIZE].pageNumber = i;
		if (i % INDEXSIZE == INDEXSIZE - 1) //jesli zapelnilismy indeks
		{
			fwrite(indexes, sizeof(Index), INDEXSIZE, indexFile);
		}
	}

	fwrite(indexes, sizeof(Index), INDEXSIZE, indexFile);
	delete[]indexes;
	fclose(indexFile);
	//dodaj minimalny klucz
	double new_table[15];
	add(0, 0, new_table);
}
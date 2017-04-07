#include <chip8.hpp>

CHIP::CHIP(const char *rom)
{
	srand (time(NULL));
	opcode = 0;
	memset((void*)memory, 0, 4096);
	memset(V, 0, 16);
	I = 0;
	pc = 0x200; /*application loaded at 0x200*/
	memset(gfx, 0, 64 * 32);
	delay_timer = 0;
	sound_timer = 0;
	memset((void*)stack, 0, 16 * 2);
	sp = 0;
	memset((void*)key, 0, 16);

	for (int i = 0; i < 80; i++)
		memory[i/* + 0x50*/] = chip8_fontset[i];

        FILE* rom_file = fopen(rom, "r");
	if (rom_file == NULL)
		exit(1);
	else
		fread((void*)&(memory[512]), 1, 0xF00 - 0x200, rom_file);

	for (int x = 0; x < 64; x++)
	{
		for (int y = 0; y < 32; y++)
		{
		        pixels[x][y].setSize(sf::Vector2f(PIXEL_WIDTH * SCALE, PIXEL_WIDTH * SCALE));
		        pixels[x][y].setPosition(x * PIXEL_WIDTH * SCALE, y * PIXEL_WIDTH * SCALE);
		        pixels[x][y].setFillColor(sf::Color::Black);
		}
	}
}

CHIP::~CHIP()
{
}

void CHIP::emulate_cycle()
{
	//Fetch opcode
	opcode = (memory[pc] << 8) | memory[pc+1];

	//Decode opcode
	switch(opcode & 0xF000)
	{
	case 0:
		switch(opcode & 0xF)
		{
		case 0:
			memset(gfx, 0, 64 * 32);
			pc += 2;
			break;
		case 0xE:
			sp--;
			pc = stack[sp];
			pc += 2;
			break;
		default:
			printf ("Unknown opcode [0x0000]: 0x%X\n", opcode);
			break;
		}
		break;
	case 0x1000:
		pc = opcode & 0x0FFF;
		break;
	case 0x2000:
		stack[sp] = pc;
		sp++;
		pc = opcode & 0x0FFF;
		break;
	case 0x3000:
		if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
			pc += 4;
		else
			pc += 2;
		break;
	case 0x4000:
		if (V[(opcode & 0x0F00) >> 8] != opcode & 0x00FF)
			pc += 4;
		else
			pc += 2;
		break;
	case 0x5000:
		if (V[(opcode & 0xF00) >> 8] == V[(opcode & 0x00F0) >> 4])
			pc += 4;
		else
			pc += 2;
		break;
	case 0x6000:
		V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
		pc += 2;
		break;
	case 0x7000:
		V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
		pc += 2;
		break;
	case 0x8000:
		switch(opcode & 0x000F)
		{
		case 0:
			V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		case 1:
			V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		case 2:
			V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		case 3:
			V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		case 4:
			if(V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8]))
				V[15] = 1; //carry
			else
				V[15] = 0;
			V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		case 5:
		        if(V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8]) 
				V[15] = 0; // there is a borrow
			else 
				V[15] = 1;					
			V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		case 6:
			V[15] = V[(opcode & 0x0F00) >> 8] & 0x1;
			V[(opcode & 0x0F00) >> 8] >>= 1;
			pc += 2;
			break;
		case 7:
		        if(V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4])
				V[15] = 0; // there is a borrow
			else
				V[15] = 1;
			V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];				
			pc += 2;
			break;
		case 0xE:
			V[15] = V[(opcode & 0x0F00) >> 8] >> 7;
			V[(opcode & 0x0F00) >> 8] <<= 1;
			pc += 2;
			break;
		default:
			printf ("Unknown opcode [0x0000]: 0x%X\n", opcode);
			break;
		}
		break;
	case 0x9000:
		if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
			pc += 4;
		else
			pc += 2;
		break;
	case 0xA000:
		I = opcode & 0x0FFF;
		pc += 2;
		break;
	case 0xB000:
		pc = V[0] + (opcode & 0x0FFF);
		break;
	case 0xC000:
		V[(opcode & 0x0F00) >> 8] = (rand() % 255) & (opcode & 0x00FF);
		break;
	case 0xD000:
	{
		unsigned short x = V[(opcode & 0x0F00) >> 8];
		unsigned short y = V[(opcode & 0x00F0) >> 4];
		unsigned short height = opcode & 0x000F;
		unsigned short pixel;
 
		V[0xF] = 0;
		for (int yline = 0; yline < height; yline++)
		{
			pixel = memory[I + yline];
			for (int xline = 0; xline < 8; xline++)
			{
				if ((pixel & (0x80 >> xline)) != 0)
				{
					if (gfx[(x + xline + ((y + yline) * 64))] == 1)
						V[0xF] = 1;                                 
					gfx[x + xline + ((y + yline) * 64)] ^= 1;
				}
			}
		}
		pc += 2;
	}
	break;
	case 0xE000:
		switch(opcode & 0xFF)
		{
		case 0x9E:
			if(key[V[(opcode & 0x0F00) >> 8]] != 0)
				pc += 4;
			else
				pc += 2;
			break;
		case 0xA1:
			if(key[V[(opcode & 0x0F00) >> 8]] == 0)
				pc += 4;
			else
				pc += 2;
			break;
		default:
			printf ("Unknown opcode [0x0000]: 0x%X\n", opcode);
		}
		break;
	case 0xF000:
		switch(opcode & 0x00FF)
		{
		case 0x0007:
			V[(opcode & 0x0F00) >> 8] = delay_timer;
			pc += 2;
			break;
		case 0x0A:
		{
			bool key_pressed = false;
			for (int i = 0; i < 16; i++)
			{
				if (key[i] != 0)
				{
					V[(opcode & 0x0F00) >> 8] = i;
					key_pressed = true;
				}
			}

			if (!key_pressed)
				return;
			pc += 2;
		}
		break;
		case 0x15:
			delay_timer = V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;
		case 0x18:
			sound_timer = V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;
		case 0x1E:      
	        	if(I + V[(opcode & 0x0F00) >> 8] > 0xFFF)
				V[0xF] = 1;
			else
				V[0xF] = 0;
			I += V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;
		case 0x29:
			I = V[(opcode & 0x0F00) >> 8] * 0x5;
			pc += 2;
			break;
		case 0x33:
			memory[I]     = V[(opcode & 0x0F00) >> 8] / 100;
			memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
			memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;
			pc += 2;
			break;
		case 0x55:
		        for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i)
				memory[I + i] = V[i];
			I += ((opcode & 0x0F00) >> 8) + 1;
			pc += 2;
			break;
		case 0x65:
			for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i)
				V[i] = memory[I + i];
			I += ((opcode & 0x0F00) >> 8) + 1;
			pc += 2;
			break;
		default:
			printf ("Unknown opcode [0x0000]: 0x%X\n", opcode);
			break;
		}
		break;
	default:
	        printf ("Unknown opcode [0x0000]: 0x%X\n", opcode);
		break;
	}

	if (delay_timer > 0)
		delay_timer--;

	if (sound_timer > 0)
	{
		if (sound_timer == 1)
			printf("BEEP!\n");
		sound_timer--;
	}
}

void CHIP::set_key(char key_num, bool state)
{
	key[key_num] = state;
	for (int i = 0; i < 16; i++)
		printf("%d", key[i]);
	printf("\n");
}

void CHIP::draw_graphics(sf::RenderWindow *window)
{
	for (int x = 0; x < 64; x++)
	{
		for (int y = 0; y < 32; y++)
		{
			if (gfx[y * 64 + x] == 1)
			        pixels[x][y].setFillColor(sf::Color::White);
			else
				pixels[x][y].setFillColor(sf::Color::Black);
			window->draw(pixels[x][y]);
		}
	}
}

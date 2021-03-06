#include "Read_Elf.h"
#define ll long long
FILE *elf=NULL;
Elf64_Ehdr elf64_hdr;
Elf64_Off code_offset;
Elf64_Addr code_segment;
Elf64_Xword code_size;
Elf64_Off data_offset;
Elf64_Addr data_segment;
Elf64_Xword data_size;
Elf64_Addr main_in;
Elf64_Addr main_end;
Elf64_Off SP_offset;
Elf64_Addr SP;
Elf64_Xword SP_size;
Elf64_Xword main_size;
Elf64_Addr gp;
Elf64_Addr a_addr;
Elf64_Addr b_addr;
Elf64_Addr c_addr;
Elf64_Addr result_addr;
Elf64_Addr sum_addr;
Elf64_Addr temp_addr;
char * all_start;


//Program headers
unsigned int padr=0;
unsigned int psize=0;
unsigned int pnum=0;

//Section Headers
unsigned int sadr=0;
unsigned int ssize=0;
unsigned int snum=0;

//Symbol table
unsigned int symnum=0;
unsigned int symadr=0;
unsigned int symsize=0;

unsigned int myindex=0;

unsigned int stradr=0;


bool open_file(const char*filename)
{
	struct stat sb;
	int fd = open(filename, O_RDONLY);
	fstat(fd, &sb);
	int file_size = sb.st_size;
	all_start = (char*)malloc(file_size);
	int ss = read(fd, all_start, file_size);
	if(ss<0)
		return false;
	return true;
}	

void read_elf(const char*filename)
{
	//printf("%s\n", filename);
	if(!open_file(filename))
	{
		printf("can't open file\n");
		exit(0);
	}
	
	memcpy(&elf64_hdr, all_start, sizeof(elf64_hdr));



	//read_Elf_header();
	
	//printf("\n\n\n\n");
	
	read_elf_sections();
	
	//printf("\n\n\n\n");
	
	read_Phdr();
	
	//printf("\n\n\n\n");
	
	read_symtable();

	//printf("\n\n\n\n");
	
	/*
	printf("-------------------Some infos------------------------------\n");
	printf("code_segment and size 0x%08llx %lldbytes\n", code_segment, code_size);
	printf("data_segment and size 0x%08llx %lldbytes\n", data_segment, data_size);
	printf("main_in and main_size 0x%08llx %lldbytes\n", main_in, main_size);
	printf("SP and SP_size 0x%08llx %lldbytes\n", SP, SP_size);
	printf("GP 0x%08llx\n", gp);
	printf("a addr:0x%08llx\n", a_addr);
	printf("b addr:0x%08llx\n", b_addr);
	printf("c addr:0x%08llx\n", c_addr);
	printf("Result addr:0x%08llx\n", result_addr);
	*/
	
}

void read_Elf_header()
{
	//printf("----------------ELF Header-----------------:\n");
	//file should be relocated
	//fread(&elf64_hdr,1,sizeof(elf64_hdr),file);
	//printf("%d\n", sizeof(elf64_hdr));
	//printf("%x\n", all_start);

	
	printf("magic number: ");
	for(int i = 0; i < 4; i++)
	{
		char mn = all_start[i];
		printf("%x ",mn);
	}

	printf("Class: ");
	switch(all_start[EI_CLASS])
	{
		case 0:
			printf("Invalid\n");
			break;
		case 1:
			printf("ELF32\n");
			break;
		case 2:
			printf("ELF64\n");
			break;
		default:
			break;
	}
	
	printf("Data: ");
	switch(all_start[EI_DATA])
	{
		case 0:
			printf("Invalid\n");
			break;
		case 1:
			printf("2's complement, little endian\n");
			break;
		case 2:
			printf("2's complement, big endian\n");
			break;
		default:
			break;
	}
		
	printf("Version: ");
	switch(all_start[EI_VERSION])
	{
		case 1:
			printf("1 current \n");
			break;
		default:
			printf("Invalid\n");
			break;
	}

	printf("OS/ABI: ");
	switch(all_start[EI_OSABI])
	{
		case 0:
			printf("unix system V ABI\n");
			break;
		case 1:
			printf("HP_UNIX\n");
			break;
		case 2:
			printf("NetBSD\n");
			break;
		case 3:
			printf("Object uses GNU ELF extensions\n");
			break;
		case 6:
			printf("Sun Solaris\n");
			break;
		case 7:
			printf("IBM AIX\n");
			break;
		case 97:
			printf("ARM\n");
			break;
		default:
			printf("some ABI version which this program don't know\n");
			break;
	}

	printf("ABI Version : %x \n", all_start[EI_ABIVERSION]);
	
	printf("Type: ");
	switch(elf64_hdr.e_type)
	{	
		case 0:
			printf("No file type\n");
			break;
		case 1:
			printf("Relocatable file \n");
			break;
		case 2:
			printf("Executable file\n");
			break;
		case 3:
			printf("Shared object file\n");
			break;
		case 4:
			printf("core file\n");
			break;
		default:
			break;
	}	
	
	printf("Machine: %x\n", elf64_hdr.e_machine);

	printf("Version: %x\n", elf64_hdr.e_version);

	printf("Entry point address:  0x%llx\n", (ll)elf64_hdr.e_entry);

	printf("Start of program headers: %lld bytes into  file\n", (ll)elf64_hdr.e_phoff);

	printf("Start of section headers: %lld bytes into  file\n", (ll)elf64_hdr.e_shoff);

	printf("Flags: 0x%x\n", elf64_hdr.e_flags);

	printf("Size of this header: %d  Bytes\n", elf64_hdr.e_ehsize);

	printf("Size of program headers: %d  Bytes\n", elf64_hdr.e_phentsize);

	printf("Number of program headers: %d  \n", elf64_hdr.e_phnum);

	printf("Size of section headers: %d   Bytes\n", elf64_hdr.e_shentsize);

	printf("Number of section headers: %d \n", elf64_hdr.e_shnum);

	printf("Section header string table index:  %d \n", elf64_hdr.e_shstrndx);
	
	
}


void read_elf_sections()
{

	//printf("------------------Elf sections-----------------\n");

	Elf64_Shdr * section_start = (Elf64_Shdr*)(all_start + elf64_hdr.e_shoff);

	char * str = all_start + section_start[elf64_hdr.e_shstrndx].sh_offset;
	/*
	for(int c=0;c<elf64_hdr.e_shnum;c++)
	{
		printf("%d %s    ",c, str + section_start[c].sh_name);
		printf("Addr: 0x%08llx   ", section_start[c].sh_addr);
		printf("offset: 0x%08llx   ", section_start[c].sh_offset);
		printf("size %x\n", section_start[c].sh_size);
 	}*/
}

void read_Phdr()
{
	//printf("------------------Elf program headers-----------------\n");
	Elf64_Phdr elf64_phdr;
	Elf64_Phdr* pro_start = (Elf64_Phdr*)(all_start+elf64_hdr.e_phoff);
	int cnt = 0;
	for(int c=0;c<elf64_hdr.e_phnum;c++)
	{
		//printf("offset:%08llx  vaddr:0x%08llx paddr:0x%08llx filesize:%lld bytes  memsize:%lldbytes\n", 
			//pro_start[c].p_offset, pro_start[c].p_vaddr, pro_start[c].p_paddr, 
			//pro_start[c].p_filesz, pro_start[c].p_memsz);
		if(pro_start[c].p_type==PT_LOAD)
		{
			if(cnt == 0)
			{
				code_offset = pro_start[c].p_offset;
				code_size = pro_start[c].p_filesz;
				code_segment = pro_start[c].p_vaddr;
			}
			else if(cnt == 1)
			{
				data_offset = pro_start[c].p_offset;
				data_segment = pro_start[c].p_vaddr;
				data_size = pro_start[c].p_filesz;
			}
			cnt++;
		}
	}
}


void read_symtable()
{
	//printf("------------------Elf Sym table-----------------\n");
	Elf64_Shdr * section_start = (Elf64_Shdr*)(all_start + elf64_hdr.e_shoff);
	char * str = all_start + section_start[elf64_hdr.e_shstrndx].sh_offset;
	Elf64_Off sym_off;
	Elf64_Xword sym_size;

	for(int c=0;c<elf64_hdr.e_shnum;c++)
	{
		if(!strcmp(str+section_start[c].sh_name, ".symtab"))
		{
			sym_off = section_start[c].sh_offset;
			sym_size = section_start[c].sh_size;
			break;
		}
 	}
 	int symnum = sym_size / sizeof(Elf64_Sym);	
	Elf64_Sym* sym_start = (Elf64_Sym*)(all_start + sym_off);
	char * symstr = all_start + section_start[elf64_hdr.e_shnum-1].sh_offset;
	for(int c=0;c<symnum;c++)
	{
		//printf("%d. 0x%08llx      %s\n",c, sym_start[c].st_value, symstr+sym_start[c].st_name);
		if(!strcmp(symstr+sym_start[c].st_name, "main"))
		{
			main_in = sym_start[c].st_value;
			main_size = sym_start[c].st_size;
			main_end = main_in + main_size;
		}
		if(!strcmp(symstr+sym_start[c].st_name, "_gp"))
		{
			gp = sym_start[c].st_value;
		}
		if(!strcmp(symstr+sym_start[c].st_name, "a"))
		{
			a_addr = sym_start[c].st_value;
		}
		if(!strcmp(symstr+sym_start[c].st_name, "b"))
		{
			b_addr = sym_start[c].st_value;
		}
		if(!strcmp(symstr+sym_start[c].st_name, "c"))
		{
			c_addr = sym_start[c].st_value;
		}
		if(!strcmp(symstr+sym_start[c].st_name, "result"))
		{
			result_addr = sym_start[c].st_value;
		}
		if(!strcmp(symstr+sym_start[c].st_name, "sum"))
		{
			sum_addr = sym_start[c].st_value;
		}
		if(!strcmp(symstr+sym_start[c].st_name, "temp"))
		{
			temp_addr = sym_start[c].st_value;
		}

	}

}

/*
int main(int argc, char * argv[])
{
	if(argc <= 1)
	{
		printf("input filename\n");
	}
	else if(argc == 2)
	{
		printf("usage\n");
		printf("-h, dump the file header\n");
		printf("-S, dump the file section\n");
		printf("-s, dump the file symble");
	}
	else if(argc == 3)
	{
		if(!open_file(argv[2]))
		{
			printf("can't open file\n");
			exit(0);
		}

		memcpy(&elf64_hdr, all_start, sizeof(elf64_hdr));

		if(!strcmp(argv[1], "-h"))
		{
			read_Elf_header();
		}
		if(!strcmp(argv[1], "-S"))
		{
			read_elf_sections();
		}
		if(!strcmp(argv[1], "-s"))
		{
			read_symtable();
		}
		if(!strcmp(argv[1], "-l"))
		{
			read_Phdr();
		}
		if(!strcmp(argv[1], "-a"))
		{
			read_elf();
		}
	}
}
*/
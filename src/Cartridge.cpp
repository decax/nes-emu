#include "Cartridge.h"
#include "FileStream.h"
#include "Rom.h"

RomHeader Cartridge::LoadRom(const char* file)
{
	FileStream fs(file, "rb");
	RomHeader romHeader;

	fs.Read((uint8*)&romHeader, sizeof(RomHeader));

	if ( !romHeader.IsValidHeader() )
		throw std::exception("Invalid romHeader");

	// Next is Trainer, if present (0 or 512 bytes)
	if ( romHeader.HasTrainer() )
		throw std::exception("Not supporting trainer roms");

	if ( romHeader.IsPlayChoice10() || romHeader.IsVSUnisystem() )
		throw std::exception("Not supporting arcade roms (Playchoice10 / VS Unisystem)");

	// Next is PRG-ROM data (16K or 32K)
	const size_t prgRomSize = romHeader.GetPrgRomSizeBytes();
	m_prgRom.Initialize(prgRomSize);
	fs.Read(m_prgRom.RawPtr(), prgRomSize);

	// Next is CHR-ROM data (8K)
	const size_t chrRomSize = romHeader.GetChrRomSizeBytes();
	m_chrRom.Initialize(chrRomSize);
	fs.Read(m_chrRom.RawPtr(), chrRomSize);

	return romHeader;
}

uint8 Cartridge::HandleCpuRead(uint16 cpuAddress)
{
	if (cpuAddress >= CpuMemory::kPrgRomBase)
	{
		return m_prgRom.Read(MapCpuToPrgRom(cpuAddress));
	}
	else if (cpuAddress >= CpuMemory::kSaveRamBase)
	{
		return m_sram.Read(MapCpuToSram(cpuAddress));
	}
	
	assert(false && "Mapper doesn't handle this address");
	return 0;
}

void Cartridge::HandleCpuWrite(uint16 cpuAddress, uint8 value)
{
	if (cpuAddress >= CpuMemory::kPrgRomBase)
	{
		m_prgRom.Write(MapCpuToPrgRom(cpuAddress), value);
		return;
	}
	else if (cpuAddress >= CpuMemory::kSaveRamBase)
	{
		m_sram.Write(MapCpuToSram(cpuAddress), value);
		return;
	}
	
	assert(false && "Mapper doesn't handle this address");
}

uint8 Cartridge::HandlePpuRead(uint16 ppuAddress)
{
	return m_chrRom.Read(MapPpuToChrRom(ppuAddress));
}

void Cartridge::HandlePpuWrite(uint16 ppuAddress, uint8 value)
{
	m_chrRom.Write(MapPpuToChrRom(ppuAddress), value);
}

uint16 Cartridge::MapCpuToPrgRom(uint16 cpuAddress)
{
	// Mapper 0 supports either 16K or 32K of PRG-ROM
	// For now, assume 16K so mirror upper bank to lower (C000-FFFF -> 8000-BFFF)
	assert(cpuAddress >= CpuMemory::kPrgRomBase);
	const uint16 kPrgRomBankSize = KB(16);
	return (cpuAddress - CpuMemory::kPrgRomBase) % kPrgRomBankSize;
}

uint16 Cartridge::MapCpuToSram(uint16 cpuAddress)
{
	assert(cpuAddress >= CpuMemory::kSaveRamBase && cpuAddress < CpuMemory::kSaveRamEnd);
	return cpuAddress - CpuMemory::kSaveRamBase;
}

uint16 Cartridge::MapPpuToChrRom(uint16 ppuAddress)
{
	assert(ppuAddress < PpuMemory::kChrRomEnd);
	// Mapper 0 only supports 8K CHR-ROM, so no mapping to do
	return ppuAddress;
}
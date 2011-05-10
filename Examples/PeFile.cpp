/*
This file is part of HadesMem.
Copyright (C) 2011 Joshua Boyce (a.k.a. RaptorFactor).
<http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

HadesMem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

HadesMem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with HadesMem.  If not, see <http://www.gnu.org/licenses/>.
*/

// C++ Standard Library
#include <string>
#include <iostream>
#include <exception>
#include <algorithm>

// Boost
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

// Windows
#include <crtdbg.h>
#include <Windows.h>

// Hades
#include "HadesCommon/I18n.hpp"
#include "HadesMemory/Memory.hpp"

int wmain(int argc, wchar_t* argv[], wchar_t* /*envp*/[])
{
  try
  {
    // Attempt to detect memory leaks in debug mode
#ifdef _DEBUG
    int CurrentFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    int NewFlags = (_CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF | 
      _CRTDBG_CHECK_ALWAYS_DF);
    _CrtSetDbgFlag(CurrentFlags | NewFlags);

    // Get default heap
    HANDLE ProcHeap = GetProcessHeap();
    if (!ProcHeap)
    {
      std::error_code const LastError = Hades::GetLastErrorCode();
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wmain") << 
        Hades::ErrorString("Could not get process heap.") << 
        Hades::ErrorCode(LastError));
    }

    // Detect heap corruption
    if (!HeapSetInformation(ProcHeap, HeapEnableTerminationOnCorruption, 
      NULL, 0))
    {
      std::error_code const LastError = Hades::GetLastErrorCode();
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wmain") << 
        Hades::ErrorString("Could not set heap information.") << 
        Hades::ErrorCode(LastError));
    }
#endif

    // Hades version number
    std::wstring const VerNum(L"TRUNK");

    // Version and copyright output
#if defined(_M_X64)
    std::wcout << "Hades PeFile AMD64 [Version " << VerNum << "]\n";
#elif defined(_M_IX86)
    std::wcout << "Hades PeFile IA32 [Version " << VerNum << "]\n";
#else
#error "[HadesMem] Unsupported architecture."
#endif
    std::wcout << "Copyright (C) 2011 RaptorFactor. All rights reserved." << 
      std::endl;
    std::wcout << "Website: http://www.raptorfactor.com/, "
      "Email: raptorfactor@raptorfactor.com." << std::endl;
    std::wcout << "Compiler: \"" << BOOST_COMPILER << "\", Standard "
      "Library: \"" << BOOST_STDLIB << "\", Platform: \"" << BOOST_PLATFORM 
      << "\", Boost: " << BOOST_VERSION << "." << std::endl;
    std::wcout << "Built on " << __DATE__ << " at " << __TIME__ << "." << 
      std::endl << std::endl;
        
    // Get target process ID from args
    if (argc < 2)
    {
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wmain") << 
        Hades::ErrorString("You must supply a valid process id."));
    }
    DWORD ProcId = 0;
    try
    {
      ProcId = boost::lexical_cast<DWORD>(argv[1]);
    }
    catch (boost::bad_lexical_cast const& /*e*/)
    {
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wmain") << 
        Hades::ErrorString("You must supply a valid process id."));
    }
    
    // Get target module from args
    if (argc < 3)
    {
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wmain") << 
        Hades::ErrorString("You must supply a target module."));
    }
    std::wstring ModuleName;
    try
    {
      ModuleName = boost::lexical_cast<std::wstring>(argv[2]);
    }
    catch (boost::bad_lexical_cast const& /*e*/)
    {
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wmain") << 
        Hades::ErrorString("You must supply a target module."));
    }
        
    // Open memory manager
    Hades::Memory::MemoryMgr MyMemory(ProcId == static_cast<DWORD>(-1) ? 
      GetCurrentProcessId() : ProcId);
        
    // Dump sections for module
    auto DumpSections = [&] (Hades::Memory::Module const& M) 
    {
      // Create PE file object for current module
      Hades::Memory::PeFile MyPeFile(MyMemory, M.GetBase());
        
      // Enumerate section list for current module
      Hades::Memory::SectionList Sections(MyPeFile);
      std::for_each(Sections.begin(), Sections.end(), 
        [&] (Hades::Memory::Section& S)
        {
          // Dump section info
          std::wcout << boost::wformat(L"S: %s - Number = %u, Name = %s, "
            L"VA = %x, VS = %x, Characteristics = %x.\n") %M.GetName() 
            %S.GetNumber() %boost::lexical_cast<std::wstring>(S.GetName()) 
            %S.GetVirtualAddress() %S.GetVirtualSize() %S.GetCharacteristics();
        });
    };
    
    // Dump exports for module
    auto DumpExports = [&] (Hades::Memory::Module const& M) 
    {
      // Create PE file object for current module
      Hades::Memory::PeFile MyPeFile(MyMemory, M.GetBase());
        
      // Enumerate export list for current module
      Hades::Memory::ExportList Exports(MyPeFile);
      std::for_each(Exports.begin(), Exports.end(), 
        [&] (Hades::Memory::Export& E)
        {
          // Output prefix
          std::wcout << "E: ";
            
          // Dump name info if available
          if (E.ByName())
          {
            std::wcout << boost::wformat(L"%s - %s!") %M.GetName() %M.GetName();
            std::cout << E.GetName();
          }
          // Otherwise dump ordinal info
          else
          {
            std::wcout << boost::wformat(L"%s - %s!%d (%x)") %M.GetName() 
              %M.GetName() %E.GetOrdinal() %E.GetOrdinal();
          }
          
          // Dump forwarder info if available
          if (E.Forwarded())
          {
            std::cout << boost::format(" -> %s\n") %E.GetForwarder();
          }
          // Otherwise dump RVA and VA info
          else
          {
            std::cout << boost::format(" -> %x (%p)\n") %E.GetRva() 
              %E.GetVa();
          }
        });
    };
    
    // Dump imports for module
    auto DumpImports = [&] (Hades::Memory::Module const& M) 
    {
      // Create PE file object for current module
      Hades::Memory::PeFile MyPeFile(MyMemory, M.GetBase());
        
      // Enumerate import dir list for current module
      Hades::Memory::ImportDirList ImpDirs(MyPeFile);
      std::for_each(ImpDirs.begin(), ImpDirs.end(), 
        [&] (Hades::Memory::ImportDir& I)
        {
          // Enumerate import thunk list for current module
          Hades::Memory::ImportThunkList ImpThunks(MyPeFile, 
            I.GetCharacteristics());
          std::for_each(ImpThunks.begin(), ImpThunks.end(), 
            [&] (Hades::Memory::ImportThunk& T)
            {
              // Output prefix
              std::wcout << "I: ";
                
              // Dump module name, import module, and import name/ordinal
              std::wcout << M.GetName() << " - ";
              if (T.ByOrdinal())
              {
                std::cout << boost::format("%s!%d\n") %I.GetName() 
                  %T.GetOrdinal();
              }
              else
              {
                std::cout << boost::format("%s!%s\n") %I.GetName() 
                  %T.GetName();
              }
            });
        });
    };
    
    // Dump all PE file info
    auto DumpAll = [&] (Hades::Memory::Module const& M) 
    {
      // Dump module name
      std::wcout << M.GetName() << "\n\n";
      
      // Create PE file object for current module
      Hades::Memory::PeFile MyPeFile(MyMemory, M.GetBase());
      
      // Get NT headers
      Hades::Memory::NtHeaders MyNtHdrs(MyPeFile);
        
      // Skip modules which don't match our architecture
#if defined(_M_X64)
      WORD MachineMe = IMAGE_FILE_MACHINE_AMD64;
#elif defined(_M_IX86)
      WORD MachineMe = IMAGE_FILE_MACHINE_I386;
#else
#error "[HadesMem] Unsupported architecture."
#endif
      if (MyNtHdrs.GetMachine() != MachineMe)
      {
        std::wcout << "WARNING: Skipping module. Architecture conflict.\n\n";
        return;
      }
      
      std::wcout << "Sections:\n";
      DumpSections(M);
      std::wcout << "\n";
      std::wcout << "Exports:\n";
      DumpExports(M);
      std::wcout << "\n";
      std::wcout << "Imports:\n";
      DumpImports(M);
      std::wcout << "\n";
    };
    
    // If module name matches 'catch-all' identifier dump info for all modules
    if (ModuleName == L".")
    {
      // Enumerate module list and dump info for all modules
      Hades::Memory::ModuleList Modules(MyMemory);
      std::for_each(Modules.begin(), Modules.end(), DumpAll);
    }
    // Otherwise find target module and dump
    else
    {
      // Find target module
      Hades::Memory::ModuleList Modules(MyMemory);
      auto Iter = std::find_if(Modules.begin(), Modules.end(), 
        [&] (Hades::Memory::Module const& M) 
        {
          return M.GetName() == ModuleName;
        });
      if (Iter == Modules.end())
      {
        BOOST_THROW_EXCEPTION(Hades::HadesError() << 
          Hades::ErrorFunction("wmain") << 
          Hades::ErrorString("Could not find target module."));
      }
      
      // Dump info for target module
      DumpAll(*Iter);
    }
    
  }
  catch (std::exception const& e)
  {
    std::cout << boost::diagnostic_information(e) << std::endl;
  }
}
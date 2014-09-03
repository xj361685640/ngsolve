#include <solve.hpp>

#ifdef TCL
#include <tcl.h>
#if TCL_MAJOR_VERSION==8 && TCL_MINOR_VERSION>=4
#define tcl_const const
#else
#define tcl_const
#endif
#endif


#include <parallelngs.hpp>

namespace ngfem
{
  // extern SymbolTable<double> * constant_table_for_FEM;
}


namespace ngsolve
{

  PDE :: PDE ()
  {
    levelsolved = -1;
    SetGood (true);
    pc = 0;
    // constant_table_for_FEM = &constants;

    AddVariable ("timing.level", 0.0, 6);


    time_t now = time(0);
    tm * lt = localtime (&now);
    stringstream date;
    date << 1900+lt->tm_year
         << "-" << setw(2) << setfill('0') << lt->tm_mon+1
         << "-" << setw(2) << lt-> tm_mday;
    AddStringConstant ("date", date.str());

    stringstream time;
    time << setw(2) << lt->tm_hour
         << ":" << setw(2) << setfill('0') << lt->tm_min 
         << ":" << setw(2) << lt-> tm_sec;
    AddStringConstant ("time", time.str());
    tcl_interpreter = NULL;
  }
  
  PDE :: ~PDE()
  {
    /*
    for (int i = 0; i < coefficients.Size(); i++)
      delete coefficients[i];
    coefficients.DeleteAll();
    for (int i = 0; i < spaces.Size(); i++)
      delete spaces[i];
    spaces.DeleteAll();
    for (int i = 0; i < gridfunctions.Size(); i++)
      gridfunctions[i].reset();
      //delete gridfunctions[i];
    // gridfunctions.DeleteAll();
    for (int i = 0; i < bilinearforms.Size(); i++)
      delete bilinearforms[i];
    bilinearforms.DeleteAll();
    for (int i = 0; i < linearforms.Size(); i++)
      delete linearforms[i];
    linearforms.DeleteAll();
    for (int i = 0; i < preconditioners.Size(); i++)
      delete preconditioners[i];
    preconditioners.DeleteAll();
    for (int i = 0; i < numprocs.Size(); i++)
      delete numprocs[i];
    numprocs.DeleteAll();
    for (int i = 0; i < variables.Size(); i++)
      delete variables[i];
    */
    for (int i = 0; i < string_constants.Size(); i++)
      delete string_constants[i];
    string_constants.DeleteAll();

    for(int i = 0; i < evaluators.Size(); i++)
      delete evaluators[i];
    evaluators.DeleteAll();

    for(int i = 0; i < CurvePointIntegratorFilenames.Size(); i++)
      delete CurvePointIntegratorFilenames[i];
    CurvePointIntegratorFilenames.DeleteAll();

    CurvePointIntegrators.DeleteAll();

    Ng_ClearSolutionData ();
    
    for (int i = 0; i < mas.Size(); i++)
      delete mas[i];
  }

  void PDE :: SavePDE (const string & filename)
  {
    ;
  } 
  
  
  void PDE :: SetFilename(const string str)
  { 
    filename = str;
#ifdef WIN32
    for(int i=0; filename[i]!=0 && i<filename.size(); i++)
      if(filename[i] == '/')
	filename[i] = '\\';
#endif
  }

  void PDE :: SaveSolution (const string & filename, const bool ascii)
  {
    // ofstream outfile(filename.c_str());
    ofstream outfile;
    if(ascii)
      outfile.open(filename.c_str());
    else
      outfile.open(filename.c_str(), ios_base::binary);

    for (int i = 0; i < gridfunctions.Size(); i++)
      {
	cout << IM(1) << "Writing gridfunction " << gridfunctions.GetName(i) << endl;

	if(gridfunctions[i]->IsUpdated())
	  {
	    if(ascii)
	      gridfunctions[i]->GetVector().SaveText (outfile);
	    else
	      gridfunctions[i]->GetVector().Save (outfile);
	  }
	else
	  {
	    cerr << "ERROR: Gridfunction \"" << gridfunctions.GetName(i) << "\" is not initialized" << endl
		 << "       => not in the output file" << endl;
	  }
      }
    outfile.close();
  }

  ///
  void PDE :: LoadSolution (const string & filename, const bool ascii)
  {
#ifdef NETGEN_ELTRANS
    abc - bin ich hier ?
    int geometryorder = 1;
    if (constants.Used ("geometryorder"))
      geometryorder = int (constants["geometryorder"]);

    bool rational = false;
    if (constants.Used ("rationalgeometry"))
      rational = bool (constants["rationalgeometry"]);

    Ng_HighOrder (geometryorder, rational);
#endif    

    ifstream infile;
    if(ascii)
      infile.open(filename.c_str());
    else
      infile.open(filename.c_str(), ios_base::binary);

    LocalHeap lh(100009, "PDE - Loadsolution");
    for (int i = 0; i < spaces.Size(); i++)
      {
	spaces[i]->Update(lh);
	spaces[i]->FinalizeUpdate(lh);
      }
    for (int i = 0; i < gridfunctions.Size(); i++)
      {
	gridfunctions[i]->Update();
	cout << IM(1) << "Loading gridfunction " << gridfunctions.GetName(i) << endl;
	if(ascii)
	  gridfunctions[i]->GetVector().LoadText (infile);
	else
	  gridfunctions[i]->GetVector().Load (infile);
      }
    infile.close();
  }


  void PDE :: PrintReport (ostream & ost) const
  { 
    ost << endl << "PDE Description:" << endl;


    for (int i = 0; i < constants.Size(); i++)
      ost << "constant " << constants.GetName(i) << " = " << constants[i] << endl;
    for (int i = 0; i < string_constants.Size(); i++)
      ost << "string constant " << string_constants.GetName(i) << " = " << string_constants[i] << endl;
    for (int i = 0; i < variables.Size(); i++)
      ost << "variable " << variables.GetName(i) << " = " << variables[i] << endl;
    for (int i = 0; i < generic_variables.Size(); i++)
      ost << "variable " << generic_variables.GetName(i) << " = " << generic_variables[i] << endl;

    ost << endl;
  
    ost << "Coefficients:" << endl
	<< "-------------" << endl;
    for (int i = 0; i < coefficients.Size(); i++)
      {
	ost << "coefficient " << coefficients.GetName(i) << ":" << endl;
	coefficients[i]->PrintReport (ost);
      }
    
    ost << endl
	<< "Spaces:" << endl
	<< "-------" << endl;
    for (int i = 0; i < spaces.Size(); i++)
      {
	ost << "space " << spaces.GetName(i) << ":" << endl;
	spaces[i]->PrintReport (ost);
      }

    ost << endl
	<< "Bilinear-forms:" << endl
	<< "---------------" << endl;
    for (int i = 0; i < bilinearforms.Size(); i++)
      {
	ost << "bilinear-form " << bilinearforms.GetName(i) << ":" << endl;
	bilinearforms[i]->PrintReport (ost);
      }

    ost << endl 
	<< "Linear-forms:" << endl
	<< "-------------" << endl;
    for (int i = 0; i < linearforms.Size(); i++)
      {
	ost << "linear-form " << linearforms.GetName(i) << ":" << endl;
	linearforms[i]->PrintReport (ost);
      }

    ost << endl 
	<< "Grid-functions:" << endl
	<< "---------------" << endl;
    for (int i = 0; i < gridfunctions.Size(); i++)
      {
	ost << "grid-function " << gridfunctions.GetName(i) << ":" << endl;
	gridfunctions[i]->PrintReport (ost);
      }

    ost << endl
	<< "Preconditioners:" << endl
	<< "----------------" << endl;
    for (int i = 0; i < preconditioners.Size(); i++)
      {
	ost << "preconditioner " << preconditioners.GetName(i) << ":" << endl;
	preconditioners[i]->PrintReport (ost);
      }

    ost << endl 
	<< "Numprocs:" << endl
	<< "---------" << endl;
    for (int i = 0; i < numprocs.Size(); i++)
      {
	ost << "numproc " << numprocs.GetName(i) << ":" << endl;
	numprocs[i]->PrintReport (ost);
      }
  }
  

  void PDE :: PrintMemoryUsage (ostream & ost)
  {
    Array<MemoryUsageStruct*> memuse;
    for (int i = 0; i < spaces.Size(); i++)
      spaces[i]->MemoryUsage (memuse);
    for (int i = 0; i < bilinearforms.Size(); i++)
      bilinearforms[i]->MemoryUsage (memuse);
    for (int i = 0; i < linearforms.Size(); i++)
      linearforms[i]->MemoryUsage (memuse);
    for (int i = 0; i < gridfunctions.Size(); i++)
      gridfunctions[i]->MemoryUsage (memuse);
    for (int i = 0; i < preconditioners.Size(); i++)
      preconditioners[i]->MemoryUsage (memuse);

    int sumbytes = 0, sumblocks = 0;
    for (int i = 0; i < memuse.Size(); i++)
      {
	ost << memuse[i]->Name() << ": " << memuse[i]->NBytes()
	    << " bytes in " << memuse[i]->NBlocks() << " blocks." << endl;
	sumbytes += memuse[i]->NBytes();
	sumblocks += memuse[i]->NBlocks();
      }
    cout << IM(1) << "total bytes " << sumbytes << " in " << sumblocks << " blocks." << endl;
  }













  bool PDE ::
  ConstantUsed (const string & aname) const
  {
    return constants.Used(aname);
  }
  
  double PDE ::
  GetConstant (const string & name, bool opt) const
  { 
    if (constants.Used(name))
      return constants[name]; 

    if (opt) return 0;
    throw Exception (string ("Constant '") + name + "' not defined\n");
  }

  bool PDE ::
  StringConstantUsed (const string & aname) const
  {
    return string_constants.Used(aname);
  }
  
  string PDE ::
  GetStringConstant (const string & name, bool opt) const
  { 
    if (string_constants.Used(name))
      return *string_constants[name]; 

    if (opt) return string("");
    throw Exception (string ("String onstant '") + name + "' not defined\n");
  }

  bool PDE ::
  VariableUsed (const string & aname) const
  {
    return variables.Used(aname);
  }
  
  double & PDE :: 
  GetVariable (const string & name, bool opt)
  { 
    if (variables.Used(name))
      return *variables[name]; 

    static double dummy;
    if (opt) return dummy;
    throw Exception (string ("Variable '") + name + "' not defined\n");
  }

  shared_ptr<CoefficientFunction> PDE :: 
  GetCoefficientFunction (const string & name, bool opt)
  { 
    if (coefficients.Used(name))
      return coefficients[name];

    if (opt) return NULL;
    throw Exception (string ("CoefficientFunction '") + name + "' not defined\n");
  }

  FESpace * PDE :: 
  GetFESpace (const string & name, bool opt)
  { 
    if (spaces.Used(name)) return spaces[name].get();

    if (opt) return NULL;
    throw Exception (string("FESpace '") + name + "' not defined\n");
  }

  GridFunction * PDE :: 
  GetGridFunction (const string & name, bool opt)
  { 
    if (gridfunctions.Used(name))
      return gridfunctions[name].get(); 

    if (opt) return NULL;
    throw Exception (string("GridFunction '") + name + "' not defined\n");
  }

  BilinearForm * PDE :: 
  GetBilinearForm (const string & name, bool opt)
  { 
    if (bilinearforms.Used(name))
      return bilinearforms[name].get(); 

    if (opt) return NULL;
    throw Exception (string("Bilinear-form '") + name + "' not defined\n");
  }

  LinearForm * PDE :: 
  GetLinearForm (const string & name, bool opt)
  {
    if (linearforms.Used(name))
      return linearforms[name].get(); 

    if (opt) return NULL;
    throw Exception (string("Linear-form '") + name + "' not defined\n");
  }

  Preconditioner * PDE :: 
  GetPreconditioner (const string & name, bool opt)
  { 
    if (preconditioners.Used(name))
      return preconditioners[name].get(); 

    if (opt) return NULL;
    throw Exception (string("Preconditioner '") + name + "' not defined\n");
  }

  NumProc * PDE :: 
  GetNumProc (const string & name, bool opt)
  { 
    if (numprocs.Used(name))
      return numprocs[name].get(); 

    if (opt) return NULL;
    throw Exception (string("Numproc '") + name + "' not defined\n");
  }

  const CoefficientFunction * PDE :: 
  GetCoefficientFunction (const string & name, bool opt) const
  { 
    if (coefficients.Used(name))
      return coefficients[name].get(); 

    if (opt) return NULL;
    throw Exception (string("CoefficientFunction '") + name + "' not defined\n");
  }

  const FESpace * PDE :: 
  GetFESpace (const string & name, bool opt) const
  { 
    if (spaces.Used(name)) return spaces[name].get();
    
    if (opt) return NULL;
    throw Exception (string("FESpace '") + name + "' not defined\n");
  }

  const GridFunction * PDE :: 
  GetGridFunction (const string & name, bool opt) const
  { 
    if (gridfunctions.Used(name))
      return gridfunctions[name].get(); 

    if (opt) return NULL;
    throw Exception (string("Grid-function '") + name + "' not defined\n");
  }

  const BilinearForm * PDE :: 
  GetBilinearForm (const string & name, bool opt) const
  { 
    if (bilinearforms.Used(name))
      return bilinearforms[name].get();

    if (opt) return NULL;
    throw Exception (string("Bilinear-form '") + name + "' not defined\n");
  }

  const LinearForm * PDE :: 
  GetLinearForm (const string & name, bool opt) const
  { 
    if (linearforms.Used(name))
      return linearforms[name].get(); 

    if (opt) return NULL;
    throw Exception (string("Linear-form '") + name + "' not defined\n");
  }

  const Preconditioner * PDE :: 
  GetPreconditioner (const string & name, bool opt) const
  { 
    if (preconditioners.Used(name))
      return preconditioners[name].get(); 

    if (opt) return NULL;
    throw Exception (string("Preconditioner '") + name + "' not defined\n");
  }

  const NumProc * PDE :: 
  GetNumProc (const string & name, bool opt) const
  { 
    if (numprocs.Used(name))
      return numprocs[name].get(); 

    if (opt) return NULL;
    throw Exception (string("Numproc '") + name + "' not defined\n");
  }




  void PDE :: Solve ()
  {
    static Timer timer("Solver - Total");
    RegionTimer reg (timer);

    size_t heapsize = 1000000;
    if (constants.Used ("heapsize"))
      heapsize = size_t(constants["heapsize"]);

#ifdef _OPENMP
    if (constants.Used ("numthreads"))
      omp_set_num_threads (int (constants["numthreads"]));
    else
      AddConstant ("numthreads", omp_get_max_threads());      
    heapsize *= omp_get_max_threads();
#endif

    LocalHeap lh(heapsize, "PDE - main heap");

    double starttime = WallTime();

    MyMPI_Barrier();

    if (pc == 0 || pc == todo.Size())
      {
        pc = 0;
        static Timer meshtimer("Mesh adaption");
        meshtimer.Start();
    
        if (levelsolved >= 0)
          {
            if (constants.Used ("refinehp"))
              Ng_Refine(NG_REFINE_HP);
            else if (constants.Used ("refinep"))
              Ng_Refine(NG_REFINE_P);
            else
              Ng_Refine(NG_REFINE_H);
          }
        
        if (constants.Used ("secondorder"))
          throw Exception ("secondorder is obsolete \n Please use  'define constant geometryorder = 2' instead");
	
        if (constants.Used ("hpref") && levelsolved == -1)
          {
            double geom_factor = constants.Used("hpref_geom_factor") ? 
              constants["hpref_geom_factor"] : 0.125;
            bool setorders = !constants.Used("hpref_setnoorders");
            
            Ng_HPRefinement (int (constants["hpref"]), geom_factor, setorders);
            /*
              if( constants.Used("hpref_setnoorders") )
              {
              if( constants.Used("hpref_geom_factor")) 
	      Ng_HPRefinement (int (constants["hpref"]),constants["hpref_geom_factor"],false);
              else
	      Ng_HPRefinement (int (constants["hpref"]),0.125 ,false);
              }
              else
              {
              if( constants.Used("hpref_geom_factor")) 
	      Ng_HPRefinement (int (constants["hpref"]),constants["hpref_geom_factor"]);
              else
	      Ng_HPRefinement (int (constants["hpref"]));
              }
            */
          }
        
        int geometryorder = 1;
        if (constants.Used ("geometryorder"))
          geometryorder = int (constants["geometryorder"]);
        
        bool rational = false;
        if (constants.Used ("rationalgeometry"))
          rational = bool (constants["rationalgeometry"]);
        
        
        if (constants.Used("common_integration_order"))
          {
            cout << IM(1) << " !!! comminintegrationorder = " << int (constants["common_integration_order"]) << endl;
            Integrator::SetCommonIntegrationOrder (int (constants["common_integration_order"]));
          }

        if (geometryorder > 1)
          Ng_HighOrder (geometryorder, rational);
        
        meshtimer.Stop();
        
        for (int i = 0; i < mas.Size(); i++)
          mas[i]->UpdateBuffers();   // update global mesh infos
      }



    cout << IM(1) << "Solve at level " << mas[0]->GetNLevels()-1
	 << ", NE = " << mas[0]->GetNE() 
	 << ", NP = " << mas[0]->GetNP() << endl;
    

    AddVariable ("mesh.levels", mas[0]->GetNLevels(), 6);
    AddVariable ("mesh.ne", mas[0]->GetNE(), 6);
    AddVariable ("mesh.nv", mas[0]->GetNV(), 6);

    // line-integrator curve points can only be built if
    // element-curving has been done
    for(int i=0; i<CurvePointIntegrators.Size(); i++)
      BuildLineIntegratorCurvePoints(*CurvePointIntegratorFilenames[i],
				     *mas[0],
				     *CurvePointIntegrators[i]);
		    

    for(  ; pc < todo.Size(); pc++)
      {
	EvalVariable * ev = dynamic_cast<EvalVariable *>(todo[pc]);
	FESpace * fes = dynamic_cast<FESpace *>(todo[pc]);
	GridFunction * gf = dynamic_cast<GridFunction *>(todo[pc]);
	BilinearForm * bf = dynamic_cast<BilinearForm *>(todo[pc]);
	LinearForm * lf = dynamic_cast<LinearForm *>(todo[pc]);
	Preconditioner * pre = dynamic_cast<Preconditioner *>(todo[pc]);
	NumProc * np = dynamic_cast<NumProc *>(todo[pc]);
        
        try
          {
            RegionTimer timer(todo[pc]->GetTimer());
            HeapReset hr(lh);

            if (ev)
              {
                cout << IM(1) << "evaluate variable " 
                     << ev->GetName() << " = " << ev->Evaluate() << endl;
              }
            
            else if (fes)
              {
		cout << IM(1)
		     << "Update " << fes -> GetClassName()
		     << " " << fes -> GetName () << flush;

		fes -> Update(lh);
		fes -> FinalizeUpdate(lh);

		int ndof = fes->GetNDofGlobal();
		AddVariable (string("fes.")+fes->GetName()+".ndof", ndof, 6);

		if (fes->GetDimension() == 1)
		  cout << IM(1) << ", ndof = " << ndof << endl;
		else
		  cout << IM(1) << ", ndof = " 
		       << fes -> GetDimension() << " x " 
		       << ndof << endl;
              }

            else if (gf)
              {
		cout << IM(1) << "Update gridfunction " << gf->GetName() << endl;
		gf->Update();
	      }
            
            else if (bf)
              {
		cout << IM(1) 
		     << "Update bilinear-form " << bf->GetName() << endl;
		bf->Assemble(lh);
	      }

            else if (lf)
              {
                if( lf->InitialAssembling() )
                  {
		    cout << IM(1) << "Update linear-form " << lf->GetName() << endl;
		    lf->Assemble(lh);
		  }
	      }
            
            else if (pre)
              {
		if ( pre->LaterUpdate() )
		  {  
		    cout << IM(1) 
			 << endl << "Update of " << pre->ClassName() 
			 << "  " << pre->GetName() << " postponed!" << endl;
		  }
		else
		  {	    
		    cout << IM(1) << "Update " << pre->ClassName() 
			 << "  " << pre->GetName() << endl;

		    pre->Update();
		  }
	      }
            else if (np)
              {
		cout << IM(1) 
		     << "Call numproc " << np->GetClassName() 
		     << "  " << np->GetName() << endl;
		
		np->Do(lh);
	      }

            else if (dynamic_cast<const LabelStatement*> (todo[pc]))
              {
                cout << IM(1)
                     << dynamic_cast<const LabelStatement*>(todo[pc])->GetLabel() << endl;
              }

            else if (dynamic_cast<const GotoStatement*> (todo[pc]))
              {
                const GotoStatement * statement =
                  dynamic_cast<const GotoStatement*>(todo[pc]);
                string target = statement -> GetTarget();
                for (int j = 0; j < todo.Size(); j++)
                  {
                    const LabelStatement * lst = 
                      dynamic_cast<const LabelStatement*> (todo[j]);
                    if (lst && lst->GetLabel() == target)
                      pc = j-1;
                  }
              }

            else if (dynamic_cast<const ConditionalGotoStatement*> (todo[pc]))
              {
                const ConditionalGotoStatement * statement =
                  dynamic_cast<const ConditionalGotoStatement*>(todo[pc]);
                string target = statement -> GetTarget();
                
                if (statement->EvaluateCondition())
                  for (int j = 0; j < todo.Size(); j++)
                    {
                      const LabelStatement * lst = 
                        dynamic_cast<const LabelStatement*> (todo[j]);
                      if (lst && lst->GetLabel() == target)
                        pc = j-1;
                    }
              }
            else if (dynamic_cast<const StopStatement*> (todo[pc]))
              {
                pc = todo.Size()-1;
              }            
            else
              cerr << "???????????????" << endl;
          }


        catch (exception & e)
          {
            throw Exception (string(e.what()) + 
                             "\nthrown by update " 
                             + todo[pc]->GetClassName() + " "
                             + todo[pc]->GetName());
          }
#ifdef _MSC_VER
# ifndef MSVC_EXPRESS
        catch (CException * e)
          {
            TCHAR msg[255];
            e->GetErrorMessage(msg, 255);
            throw Exception (msg + 
                             string ("\nthrown by update ")
                             + todo[pc]->GetClassName() + " "
                             + todo[pc]->GetName());
          }
# endif // MSVC_EXPRESS
#endif
        catch (Exception & e)
          {
            e.Append ("\nthrown by update ");
            e.Append (todo[pc]->GetClassName());
            e.Append (" ");
            e.Append (todo[pc]->GetName());
            throw;
          }
        
	AddVariable ("timing.level", WallTime()-starttime, 6);
        if (bf) AddVariable (string("timing.bf.")+bf->GetName(), bf->GetTimer().GetTime(), 6);
        if (lf) AddVariable (string("timing.lf.")+lf->GetName(), lf->GetTimer().GetTime(), 6);
        if (np) AddVariable (string("timing.np.")+np->GetName(), np->GetTimer().GetTime(), 6);
      }

#ifndef NGS_PYTHON
    // we want to keep objects in python
    for (int i = 0; i < preconditioners.Size(); i++)
      if(!preconditioners[i]->SkipCleanUp())
	preconditioners[i]->CleanUpLevel();
    for (int i = 0; i < bilinearforms.Size(); i++)
      if(!bilinearforms[i]->SkipCleanUp())
	bilinearforms[i]->CleanUpLevel();
    for (int i = 0; i < linearforms.Size(); i++)
      if(!linearforms[i]->SkipCleanUp())
	linearforms[i]->CleanUpLevel();
#endif
    // set solution data
    // for (int i = 0; i < gridfunctions.Size(); i++)
    //   gridfunctions[i]->Visualize(gridfunctions.GetName(i));


    Ng_Redraw();
    levelsolved++;
    
    MyMPI_Barrier();
    double endtime = WallTime();
    
    cout << IM(1) << "Equation Solved" << endl;
    cout << IM(1) << "Total Time = " << endtime-starttime << " sec wall time" << endl << endl;
  }






  void PDE :: DoArchive (Archive & archive)
  {
    LocalHeap lh (1000000, "ArchiveHeap");

    archive & geometryfilename & meshfilename;

    if (archive.Output())
      {
        mas[0] -> ArchiveMesh (archive);
      }
    else
      {
        MeshAccess * ma = new MeshAccess;
        ma -> ArchiveMesh (archive);
        AddMeshAccess(ma);
      }


    archive & constants;
    archive & string_constants;
    archive & variables;

    // archive & evaluators;
    // archive & coefficients;

    // archive spaces
    if (archive.Output())
      {
        archive << spaces.Size();
        for (int i = 0; i < spaces.Size(); i++)
          {
            archive << string(spaces.GetName(i));
            archive << spaces[i] -> type;
            archive << spaces[i] -> GetDimension();
            spaces[i] -> DoArchive(archive);
          }
      }
    else
      {
        int size;
        archive & size;
        for (int i = 0; i < size; i++)
          {
            string type, name;
            int dim;
            archive & name & type & dim;

            Flags flags;
            flags.SetFlag ("dim",dim);
            FESpace * fes = CreateFESpace (type, *mas[0], flags);

            fes -> DoArchive(archive);
            fes -> FinalizeUpdate (lh);
            spaces.Set (name, shared_ptr<FESpace>(fes));
            todo.Append(fes);
          }
      }

    // archive gridfunctions
    if (archive.Output())
      {
        archive << gridfunctions.Size();
        for (int i = 0; i < gridfunctions.Size(); i++)
          {
            archive << string (gridfunctions.GetName(i));
            archive << gridfunctions[i]->GetFESpace().GetName();
            gridfunctions[i] -> DoArchive (archive);
            // cout << "archive gf, type = " << typeid(*gridfunctions[i]).name() << endl;
          }
      }
    else
      {
        int s;
        archive & s;
        for (int i = 0; i < s; i++)
          {
            string name, fesname;
            archive & name & fesname;
            GridFunction * gf = CreateGridFunction (GetFESpace(fesname), name, Flags());
            // cout << "got gf, type = " << typeid(*gf).name() << endl;
            AddGridFunction (name, shared_ptr<GridFunction> (gf));
            gridfunctions[i] -> DoArchive (archive);            
          }
      }
  }



  void PDE :: AddConstant (const string & name, double val)
  {
    cout << IM(1) << "add constant " << name << " = " << val << endl;
    constants.Set (name.c_str(), val);
  }

  void PDE :: AddStringConstant (const string & name, const string & val)
  {
    cout << IM(1) << "add string constant " << name << " = " << val << endl;
    if(string_constants.Used(name))
      delete string_constants[name];

    string_constants.Set (name.c_str(), new string(val));

    if (name == "testout") testout = new ofstream (val.c_str());
  }

  void PDE :: AddVariable (const string & name, double val, int im)
  {
    cout << IM(im) << "add variable " << name << " = " << val << endl;
    if (variables.Used(name))
      *variables[name] = val;
    else
      {
	double * varp = new double;
	*varp = val;
	variables.Set (name, shared_ptr<double> (varp));
      }
  }

  
  void PDE :: AddVariable (const string & name, EvalVariable * eval)
  {
    evaluators.Append(eval);
    todo.Append(eval);
    // variables.Set (name, 0);
    AddVariable (name, 0.0);
    eval->SetVariable(*variables[name]);
    cout << IM(1) << "add variable " << name << " = " << eval->Evaluate() << endl;
  }

  void PDE :: AddVariableEvaluation (EvalVariable * eval)
  {
    evaluators.Append(eval);
    todo.Append(eval);
  }

  void PDE :: AddCoefficientFunction (const string & name, shared_ptr<CoefficientFunction> fun)
  {
    cout << IM(1) << "add coefficient-function, name = " << name << endl;
    coefficients.Set (name.c_str(), fun);
  }




  FESpace * PDE :: AddFESpace (const string & name, const Flags & hflags)
  {
    cout << IM(1) << "add fespace " << name << endl;

    Flags flags = hflags;

    FESpace * space = 0;
    
    int meshnr = int (flags.GetNumFlag ("mesh", 1)) - 1;
    const MeshAccess & ma = GetMeshAccess (meshnr);

    if (flags.GetDefineFlag ("vec"))
      flags.SetFlag ("dim", ma.GetDimension());
    if (flags.GetDefineFlag ("tensor")) 
      flags.SetFlag ("dim", sqr (ma.GetDimension()));
    if (flags.GetDefineFlag ("symtensor")) 
      flags.SetFlag ("dim", ma.GetDimension()*(ma.GetDimension()+1) / 2);

    string type = flags.GetStringFlag("type", "");
    
    space = CreateFESpace (type, ma, flags);
    
    if (type == "compound" || flags.GetDefineFlag ("compound"))
      {
        const Array<char*> & spacenames = flags.GetStringListFlag ("spaces");
        cout << IM(1) << "   spaces = " << spacenames << endl;
        
        Array<FESpace*> cspaces (spacenames.Size());
        for (int i = 0; i < cspaces.Size(); i++)
          cspaces[i] = GetFESpace (spacenames[i]);
        
        space = new CompoundFESpace (GetMeshAccess(), cspaces, flags);
      }
    if (!space) 
      {
        stringstream out;
        out << "unknown space type " << type << endl;
        out << "available types are" << endl;
        GetFESpaceClasses().Print (out);
        out << "compound\n" << endl;
	
        throw Exception (out.str());
      }

    
    if (flags.NumListFlagDefined ("dirichletboundaries"))
      {
	BitArray dirbnds(ma.GetNBoundaries());
	dirbnds.Clear();
	const Array<double> & array = flags.GetNumListFlag ("dirichletboundaries");
	for (int i = 0; i < array.Size(); i++)
	  dirbnds.Set (int(array[i])-1);
	space->SetDirichletBoundaries (dirbnds);
      }
  
    if (flags.NumListFlagDefined ("domains"))
      {
	BitArray definedon(ma.GetNDomains());
	definedon.Clear();
	const Array<double> & domains = flags.GetNumListFlag ("domains");
	for (int i = 0; i < domains.Size(); i++)
	  definedon.Set (int(domains[i])-1);
	space->SetDefinedOn (definedon);
      }

    if (flags.NumListFlagDefined ("boundaries"))
      {
	BitArray definedon(ma.GetNBoundaries());
	definedon.Clear();
	const Array<double> & boundaries = flags.GetNumListFlag ("boundaries");
	for (int i = 0; i < boundaries.Size(); i++)
	  definedon.Set (int(boundaries[i])-1);
	space->SetDefinedOnBoundary (definedon);
      }

    space->SetName (name);
    spaces.Set (name, shared_ptr<FESpace>(space));
    todo.Append(space);
    AddVariable (string("fes.")+space->GetName()+".ndof", 0.0, 6);

    return space;
  }

  void PDE :: AddFESpace (const string & name, FESpace * space)
  {
    space->SetName (name);
    spaces.Set (name, shared_ptr<FESpace>(space));
    todo.Append(space);
  }




  GridFunction * PDE :: AddGridFunction (const string & name, const Flags & flags)
  {
    cout << IM(1) << "add grid-function " << name << endl;

    string spacename = flags.GetStringFlag ("fespace", "");

    if (!spaces.Used (spacename))
      {
	throw Exception (string ("Gridfuncton '") + name +
			 "' uses undefined space '" + spacename + "'");
      }

    const FESpace * space = GetFESpace(spacename);
 
    GridFunction * gf = CreateGridFunction (space, name, flags);

    AddGridFunction (name, shared_ptr<GridFunction>(gf), true); // flags.GetDefineFlag ("addcoef"));
    return gf;
  }

  void PDE :: AddGridFunction (const string & name, shared_ptr<GridFunction> gf, bool addcf)
  {
    gf -> SetName (name);
    gridfunctions.Set (name, gf);
    todo.Append(gf.get());

    if (addcf && (gf->GetFESpace().GetIntegrator()||gf->GetFESpace().GetEvaluator()) )
      AddCoefficientFunction (name, shared_ptr<CoefficientFunction> (new GridFunctionCoefficientFunction(*gf)));
    
    if (addcf && gf->GetFESpace().GetFluxEvaluator())
      {
        const DifferentialOperator * diffop = gf->GetFESpace().GetFluxEvaluator();
        string fluxname = diffop->Name() + "_" + name;
        AddCoefficientFunction (fluxname, shared_ptr<CoefficientFunction> (new GridFunctionCoefficientFunction(*gf, diffop)));
      }
    

    const CompoundFESpace * cfe = dynamic_cast<const CompoundFESpace*>(&gf->GetFESpace());
    if (cfe)
      {
	for (int i = 0; i < cfe->GetNSpaces(); i++)
	  {
            string nname = name + "." + ToString(i+1);
	    AddGridFunction (nname, shared_ptr<GridFunction>(gf->GetComponent(i)), addcf);
	  }
      }
  }


  BilinearForm * PDE :: AddBilinearForm (const string & name, const Flags & flags)
  {
    cout << IM(1) << "add bilinear-form " << name << endl;
    string spacename = flags.GetStringFlag ("fespace", "");

    if (!spaces.Used (spacename))
      {
	cerr << "space " << spacename << " not defined " << endl;
	return 0;
      }
    const FESpace * space = spaces[spacename].get();

    const FESpace * space2 = NULL;
    if (flags.StringFlagDefined ("fespace2"))
      space2 = spaces[flags.GetStringFlag ("fespace2", "")].get();
  
    if (!space2)
      {
	bilinearforms.Set (name, shared_ptr<BilinearForm>(CreateBilinearForm (space, name, flags)));
      }
    else
      bilinearforms.Set (name, shared_ptr<BilinearForm>(new T_BilinearForm<double > (*space, *space2, name, flags)));
    
    if (flags.StringFlagDefined ("linearform"))
      bilinearforms[name] -> SetLinearForm (GetLinearForm (flags.GetStringFlag ("linearform", 0)));

    todo.Append(bilinearforms[name].get());

    return bilinearforms[name].get();
  }


 
  LinearForm * PDE :: AddLinearForm (const string & name, const Flags & flags)
  {
    cout << IM(1) << "add linear-form " << name << endl;

    string spacename = flags.GetStringFlag ("fespace", "");

    if (!spaces.Used (spacename))
      {
	throw Exception (string ("Linear-form '") + name +
			 "' uses undefined space '" + spacename + "'");
      }

    const FESpace * space = spaces[spacename].get();

    linearforms.Set (name, shared_ptr<LinearForm> (CreateLinearForm (space, name, flags)));
    todo.Append(linearforms[name].get());

    return linearforms[name].get();
  }



  Preconditioner * PDE :: AddPreconditioner (const string & name, const Flags & flags)
  {
    cout << IM(1) << "add preconditioner " << name << flush;

    Preconditioner * pre = NULL;
    const char * type = flags.GetStringFlag ("type", NULL);

    int ntasks = MyMPI_GetNTasks ();
    
    if ( ntasks == 1 )
      {
	/*
	if (strcmp (type, "multigrid") == 0)
	  pre = new MGPreconditioner (this, flags, name);
	else 
	if (strcmp (type, "direct") == 0)
	  pre = new DirectPreconditioner (this, flags, name);
	else 
	*/

	if (strcmp (type, "local") == 0)
	  pre = new LocalPreconditioner (this, flags, name);
	
	else if (strcmp (type, "twolevel") == 0)
	  pre = new TwoLevelPreconditioner (this, flags, name);
	
	else if (strcmp (type, "complex") == 0)
	  pre = new ComplexPreconditioner (this, flags, name);
	
	else if (strcmp (type, "chebychev") == 0)
	  pre = new ChebychevPreconditioner (this, flags, name);
	
	//  else if (strcmp (type, "constrained") == 0)
	//    pre = new ConstrainedPreconditioner (this, flags);
	
	else if (strcmp (type, "amg") == 0)
	  pre = new CommutingAMGPreconditioner (this, flags, name);
	
	// changed 08/19/2003, Bachinger
	else if (strcmp (type, "nonsymmetric") == 0)
	  pre = new NonsymmetricPreconditioner (this, flags, name);

	else
	  for (int i = 0; i < GetPreconditionerClasses().GetPreconditioners().Size(); i++)
	    {
	      if (flags.GetDefineFlag (GetPreconditionerClasses().GetPreconditioners()[i]->name))
		pre = GetPreconditionerClasses().GetPreconditioners()[i]->creator (*this, flags, name);
	      
	      if (string(type) == GetPreconditionerClasses().GetPreconditioners()[i]->name)
		pre = GetPreconditionerClasses().GetPreconditioners()[i]->creator (*this, flags, name);
	    }
      }
    else  // parallel computations
      {
	/*
	if (strcmp (type, "multigrid") == 0)
	  {
	    pre = new MGPreconditioner (this, flags, name);
	  }
	else 
	*/
	if (strcmp (type, "local") == 0)
	  pre = new LocalPreconditioner (this, flags, name);
	/*
	  else if (strcmp (type, "direct") == 0)
	  pre = new DirectPreconditioner (this, flags, name);
	*/
	else if (strcmp (type, "twolevel") == 0)
	  pre = new TwoLevelPreconditioner (this, flags, name);

	else if (strcmp (type, "complex") == 0)
	  pre = new ComplexPreconditioner (this, flags, name);
	
	else if (strcmp (type, "chebychev") == 0)
	  pre = new ChebychevPreconditioner (this, flags, name);
	
	//  else if (strcmp (type, "constrained") == 0)
	//    pre = new ConstrainedPreconditioner (this, flags);
	
	else if (strcmp (type, "amg") == 0)
	  pre = new CommutingAMGPreconditioner (this, flags, name);
	
	// changed 08/19/2003, Bachinger
	else if (strcmp (type, "nonsymmetric") == 0)
	  pre = new NonsymmetricPreconditioner (this, flags, name);
	else
	  for (int i = 0; i < GetPreconditionerClasses().GetPreconditioners().Size(); i++)
	    {
	      if (flags.GetDefineFlag (GetPreconditionerClasses().GetPreconditioners()[i]->name))
		pre = GetPreconditionerClasses().GetPreconditioners()[i]->creator (*this, flags, name);

	      if (string(type) == GetPreconditionerClasses().GetPreconditioners()[i]->name)
		pre = GetPreconditionerClasses().GetPreconditioners()[i]->creator (*this, flags, name);
	      
	      
	    }
      }

	
    if (!pre)
      throw Exception ("Unknown preconditioner type " + string(type));
    
    preconditioners.Set (name, shared_ptr<Preconditioner> (pre));
    cout << IM(1) << ", type = " << pre->ClassName() << endl;
    
    if (!flags.GetDefineFlag ("ondemand"))
      todo.Append(pre);
    
    return pre;
  }



  void PDE :: AddNumProc (const string & name, shared_ptr<NumProc> np)
  {
    cout << IM(1) << "add numproc " << name << ", type = " << np->GetClassName() << endl;
    np->SetName (name);
    numprocs.Set (name, np);
    todo.Append(np.get());
  }



  
  void PDE :: AddBilinearFormIntegrator (const string & name, shared_ptr<BilinearFormIntegrator> part,
					 const bool deletable)
  {
    BilinearForm * form = GetBilinearForm (name);
    if (form && part)
      {
	form->AddIntegrator (part,deletable);
	cout << IM(1) << "integrator " << part->Name() << endl;
      }
    else
      {
	cerr << IM(1) << "Bilinearform = " << form << ", part = " << part << endl;
      }
  }
  
  /*
  void PDE :: AddIndependentBilinearFormIntegrator (const string & name, BilinearFormIntegrator * part,
						    const int master, const int slave,
						    const bool deletable)
  {
    BilinearForm * form = GetBilinearForm (name);
    if (form && part)
      {
	form->AddIndependentIntegrator (part,master,slave,deletable);
	cout << IM(1) << "integrator " << part->Name() << endl;
      }
    else
      {
	cerr << IM(1) << "Bilinearform = " << form << ", part = " << part << endl;
      }
  }
  */
 
  void PDE :: AddLinearFormIntegrator (const string & name, shared_ptr<LinearFormIntegrator> part)
  {
    LinearForm * form = GetLinearForm (name);
    if (form && part)
      {
	form->AddIntegrator (part);
	cout << IM(1) << "integrator " << part->Name() << endl;
      }
    else
      {
	cerr << IM(1) << "Linearform = " << form << ", part = " << part << endl;
      }
  }
  
  void PDE :: SetLineIntegratorCurvePointInfo(const string & filename,
					      Integrator * integrator)
  {
    CurvePointIntegrators.Append(integrator);
    CurvePointIntegratorFilenames.Append(new string(filename));
  }

  void PDE :: WritePDEFile ( string abspdefile, string geofile, 
			     string meshfile, string matfile, string oldpdefile )
  {
    ofstream pdeout ( abspdefile.c_str() );
    ifstream pdein ( oldpdefile.c_str() );

    pdeout << "geometry = " << geofile << endl;
    pdeout << "mesh = " << meshfile << endl;
    if ( matfile != "" )
      pdeout << "matfile = " << matfile << endl;

    string token;
    char ch;

    bool init = true;
    while ( init )
      {
	pdein.get(ch);
	if ( ch == '\n' )
	  continue;
	else if ( ch == '#' )
	  {
	    while ( ch != '\n' )
	      pdein.get(ch);
	    continue;
	  }
	pdein.putback(ch);
	pdein >> token;
	if ( token == "mesh" || token == "geometry" || token == "matfile" )
	  {
	    while ( ch != '\n' )
	      pdein.get(ch);
	    continue;
	  }
	pdeout << token;
	init = false;
      }

    // copy rest of file
    
    while ( pdein.good())
      {
	pdein.get(ch);
	pdeout.put(ch);
      }
  }




  void PDE :: Tcl_Eval (string str)
  {
#ifdef TCL
    if (!tcl_interpreter) return;
    ::Tcl_Eval (tcl_interpreter, str.c_str());
#else
    cout << "sorry, no Tcl" << endl;
#endif
    
    /*
      // for non-const tcl_eval (Tcl 8.3)
    char *dummy; 
    dummy = new char[str.size()+1];
    strcpy(dummy, str.c_str());
    
    ::Tcl_Eval (tcl_interpreter, dummy);

    delete [] dummy;
    */
  }

  /*
  void PDE :: Tcl_CreateCommand(Tcl_Interp *interp,
			   CONST char *cmdName, Tcl_CmdProc *proc,
			   ClientData clientData,
			   Tcl_CmdDeleteProc *deleteProc);
  */


#ifdef ASTRID
  void PDE :: SaveZipSolution (const string & filename, const bool ascii )
  {
    string::size_type pos1 = filename.rfind('\\');
    string::size_type pos2 = filename.rfind('/');
    
    if (pos1 == filename.npos) pos1 = 0;
    if (pos2 == filename.npos) pos2 = 0;

    string oldmatfile = this -> GetMatfile();
    string oldpdefile = this -> GetFilename();
    string oldgeofile    = this -> GetGeoFileName();

    string pde_directory = filename.substr (0, max2(pos1, pos2));
    string dirname = filename.substr (max2(pos1, pos2) +1, filename.length() - max2(pos1,pos2) - 8);
    string absdirname = pde_directory +"/" + dirname;

    string meshfile = dirname + ".vol";
    string absmeshfile = absdirname + "/" + meshfile;
    string pdefile = dirname + ".pde";
    string geofile = dirname + ".geo";
    string solfile = dirname + ".sol";
    string abspdefile = absdirname + "/" + pdefile;
    string absgeofile = absdirname + "/" + geofile;
    string abssolfile = absdirname + "/" + solfile;
    string absmatfile = "";
    string matfile = "";


    string mkdir = "mkdir " + absdirname;
    system ( mkdir.c_str() );


    string cpgeo = "cp " + oldgeofile + " " + absgeofile;

    system ( cpgeo.c_str() );


    if ( oldmatfile != "" )
      {
	matfile = dirname + ".mat";
	absmatfile = absdirname + "/" + matfile;
	string cpmat = "cp " + oldmatfile + " " + absmatfile;
	system (cpmat.c_str() );
      }

    // save current mesh
    Ng_SaveMesh(absmeshfile.c_str());
    // write pdefile with changed geo, mesh and matfile
    (*this).WritePDEFile ( abspdefile, geofile, meshfile, matfile, oldpdefile );
    // save solution to .sol file
    (*this).SaveSolution ( abssolfile, ascii ); 

    string tarfiles = "cd " + pde_directory + "\ntar -cf " + dirname + ".tar " + dirname;
    //pdefile + " " + geofile + " " + matfile + " " + solfile + " " + meshfile; 
    string gzipfiles = "cd " + pde_directory + "\ngzip -f " + dirname + ".tar";
    //\nrm "  + pdefile + " " + geofile + " " + matfile + " " + solfile + " " + meshfile; 
    string rmdir = "cd " + absdirname + "\nrm " + pdefile + " " +  geofile + " " + matfile + " " + solfile + " " + meshfile + "\ncd ..\nrmdir " + dirname; 
    system (tarfiles.c_str() );
    system (gzipfiles.c_str() );
    system ( rmdir.c_str() );

    cout << IM(1) << "saved geometry, mesh, pde and solution to file " << endl 
	 << pde_directory << "/" 
	 << dirname << ".tar.gz" << endl;
  }
  ///
  void PDE :: LoadZipSolution (const string & filename, const bool ascii)
  { 
    string::size_type pos1 = filename.rfind('\\');
    string::size_type pos2 = filename.rfind('/');
  
    if (pos1 == filename.npos) pos1 = 0;
    if (pos2 == filename.npos) pos2 = 0;

    string pde_directory = filename.substr (0, max2(pos1, pos2));
    string dirname = filename.substr (max2(pos1, pos2) +1, filename.length() - max2(pos1,pos2) - 8);
    string absdirname = pde_directory +"/" + dirname;
  
    string pdefile = dirname + "/" + dirname + ".pde";
    string solfile = dirname + "/" + dirname + ".sol";
    string abspdefile = pde_directory + "/" + pdefile;
    string abssolfile = pde_directory + "/" + solfile;
  
    string unzipfiles =  "cd " + pde_directory + "\ngzip -d " + dirname + ".tar.gz";
    string untarfiles =  "cd " + pde_directory + "\ntar -xf " + dirname + ".tar";
    string rmtar = "rm " + absdirname + ".tar";

    system (unzipfiles.c_str() );
    system (untarfiles.c_str() );
    system (rmtar.c_str());

    (*this).LoadPDE(abspdefile, 0, 0);
    (*this).LoadSolution(abssolfile, ascii );
  }
#endif
  ///

}

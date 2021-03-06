/** @file lce_breed.h
*
*   Copyright (C) 2006 Frederic Guillaume    <guillaum@zoology.ubc.ca>
*   Copyright (C) 2008 Samuel Neuenschwander <samuel.neuenschwander@unil.ch>
*
*   quantiNEMO:
*   quantiNEMO is an individual-based, genetically explicit stochastic
*   simulation program. It was developed to investigate the effects of
*   selection, mutation, recombination, and drift on quantitative traits
*   with varying architectures in structured populations connected by
*   migration and located in a heterogeneous habitat.
*
*   quantiNEMO is built on the evolutionary and population genetics
*   programming framework NEMO (Guillaume and Rougemont, 2006, Bioinformatics).
*
*
*   Licensing:
*   This file is part of quantiNEMO.
*
*   quantiNEMO is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   quantiNEMO is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with quantiNEMO.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef lce_breedH
#define lce_breedH
#include "lifecycleevent.h"

#include "random.h"
#include "tselection.h"

class TSelection;

// Class LCE_Breed
//
/**Class for the breeding (and mating) life cycle events.
   This class registers the \a mating_system, \a mating_proportion, and \a mean_fecundity
   parameters to the \c ParamSet. Sets the mating function variable to the right mating function.

   Implementation of the basic breeding and mating procedures, does not link to any trait.
   Individuals mate according to the mating system chosen. The number of offspring by female is driven
   from a Poisson distribution with mean equal to the \a mean_fecundity parameter value. The mated adults
   are not removed from the population. The offspring containers are filled with the new generation. Note that
   they are first emptied if still containing offspring individuals.

   The population's age is set to \c ALL. The adults mating and realized fecundity counters
   are updated.
**/
class LCE_Breed : public LCE  {

public:
  LCE_Breed(int rank = my_NAN);

  virtual ~LCE_Breed   ( ) {
		if(_aMatingPairs[MAL]) delete[] _aMatingPairs[MAL];
    if(_aMatingPairs[FEM]) delete[] _aMatingPairs[FEM];
  }

protected:
  void (LCE_Breed::* breed)();

  void create_mating_pairs(Patch* cur_patch, unsigned int& nbPairs);
  void createOffspring(Patch* cur_patch, unsigned int nbDaugthers, unsigned int nbSons);

	Individual**  _aMatingPairs[2];      // 0: the male, 1: the female (used for the monogamy mating system
  unsigned int  _aMatingPairs_size;    // the size of aMatingPairs

  int           _mating_system;         // 0: random mating (hermaphrodite, selfing by chance allowed (1/N))(default)
																				// 1: selfing       (hermaphrodite, selfing rate depends on parameter mating_proportion)
			                                  // 2: promiscuity / random mating (two sexes)
			                                  // 3: polygyny
			                                  // 4: monogamy
  double        _threshold;

	unsigned int  _mating_males;
  double        _mating_proportion;
  double        _mean_fecundity;
  double        _growth_rate;
	double        _sex_ratio;         // saved as males/(males+females); input as males/females
  int           _breed_model;       // 0: soft selection:      fitness relative to patch (default)
                                    // 1: soft/hard selection: fitness relative to metapopulation
                                    // 2: hard selection:      fitness directly translated to reproduction success
                                    // 3: neutral;

  int           _nbOffspring_model; // 0: carrying capacity:	            nbOffs = carryingCapacity (default)
                                    // 1: keep number                     nbOffs = nbAdult
                                    // 2: fecundity: 		                  nbOffs = nbFemales*meanFecundity, but max K
                                    // 3: fecundity (stochastic):         nbOffs = ranPossion(nbFemales*meanFecundity), but max K
                                    // 4: logistic regulated: 	          nbOffs = logisticGrowth(nbAdults, carrying capacity)
                                    // 5: logistic regulated (stochastic):nbOffs = Poisson(logisticGrowth(nbAdults, carrying capacity))

  // temporaire varaibles
  unsigned int _nbIndividuals[2]; // current number of females and males

  // temporare array for each sex when selection occurs (male: 0; female: 1)
  int           _sort[2]; // how to use the fitness: 0: get random fittest;
                          //                         1: get random fittest of a subset of fittest;
                          //                         2: get random less fittest;
                          //                         3: get random less fittest of a subset of less fittest;

  Individual* (LCE_Breed::* getMother_func_ptr)(Patch*, unsigned int&, sex_t sex);///< A pointer to a mating function for females
  Individual* (LCE_Breed::* getFather_func_ptr)(Patch*, unsigned int&, sex_t sex);///< A pointer to a mating function for males

  /** function to set the number of females and males and checks if the conditions are met for mating */
  bool (LCE_Breed::* isMatingPossible_func_ptr)(Patch*);
  bool isMatingPossbile_1_sex(Patch* cur_patch){
    _nbIndividuals[FEM] = cur_patch->size(FEM, ADLTx);
    assert(!cur_patch->size(MAL, ADLTx));
    _nbIndividuals[MAL] = 0;            // no males are present
		if(cur_patch->size(OFFSx)) cur_patch->flush(OFFSx);
		return _nbIndividuals[FEM] ? true : false;
	}
	bool isMatingPossible_2_sex(Patch* cur_patch){
		_nbIndividuals[FEM] = cur_patch->size(FEM, ADLTx);
		_nbIndividuals[MAL] = cur_patch->size(MAL, ADLTx);
    if(cur_patch->size(OFFSx)) cur_patch->flush(OFFSx);
    return (_nbIndividuals[FEM] && _nbIndividuals[MAL]) ? true : false;
  }

  /** function to compute the number of sons/daugthers depending on the total number of childs
    * nbSons and nbDaughters are changed
    * nbBaby is the total number of new offspring
    * nbMAL and nbFEM are the number of females and males currently in the patch
    *     (nbMAL and nbFEM are used to compute the current sex ratio)
    */
  void (LCE_Breed::* setSexRatio_func_ptr)(const unsigned int&, unsigned int&, unsigned int&, const unsigned int&, const unsigned int&);
  void setSexRatio_Selfing(const unsigned int& nbBaby, unsigned int& nbSons, unsigned int& nbDaugthers, const unsigned int& nbMAL, const unsigned int& nbFEM){
    nbSons = 0;
    nbDaugthers = nbBaby;
  }
  void setSexRatio_NoSelfing(const unsigned int& nbBaby, unsigned int& nbSons, unsigned int& nbDaugthers, const unsigned int& nbMAL, const unsigned int& nbFEM){
		nbSons = my_round(SimRunner::r.Binomial(_sex_ratio, nbBaby));
		nbDaugthers = nbBaby - nbSons;
  }
  void setSexRatio_KeepSexRatio(const unsigned int& nbBaby, unsigned int& nbSons, unsigned int& nbDaugthers, const unsigned int& nbMAL, const unsigned int& nbFEM){
    nbSons = my_round(nbBaby*((double)nbMAL)/(nbMAL+nbFEM));
    nbDaugthers = nbBaby - nbSons;
  }

  sex_t (LCE_Breed::* getRandomSex_func_ptr)(const unsigned int&, const unsigned int&);
  sex_t getRandomSex_Selfing(const unsigned int& nbMAL, const unsigned int& nbFEM){
    return FEM;
  }
  sex_t getRandomSex_NoSelfing(const unsigned int& nbMAL, const unsigned int& nbFEM){
		if(SimRunner::r.Uniform()<_sex_ratio) return MAL;
		else                                  return FEM;
  }
  sex_t getRandomSex_KeepSexRatio(const unsigned int& nbMAL, const unsigned int& nbFEM){
		if(SimRunner::r.Uniform()<(nbMAL/(double)(nbMAL+nbFEM))) return MAL;
		else                                                     return FEM;
  }

	sex_t _maleSex;    // normaly MAL, but when only one sex is used this is FEM

	TSelection* _pSelection; // do not delete this object here (it belongs to the metapop)

public:

	// function to compute the number of daugthers and sons (returns the total number of offspring)
  unsigned int (LCE_Breed::* setNbOffspring_func_ptr) (const unsigned int&, const unsigned int&, const unsigned int&);
	unsigned int setNbOffspring_KeepNb(const unsigned int& nbMAL, const unsigned int& nbFEM, const unsigned int& K){
    return nbMAL+nbFEM;
  }
  unsigned int setNbOffspring_CarryCapacity(const unsigned int& nbMAL, const unsigned int& nbFEM, const unsigned int& K){
    return K;
  }
  unsigned int setNbOffspring_Logistic(const unsigned int& nbMAL, const unsigned int& nbFEM, const unsigned int& K){
    // return logisticGrowth(_growth_rate, K, nbMAL+nbFEM);
		return my_round(beverton_hold(_growth_rate, K, nbMAL+nbFEM));
  }
  unsigned int setNbOffspring_RandLogistic(const unsigned int& nbMAL, const unsigned int& nbFEM, const unsigned int& K){
		// return SimRunner::r.Poisson(logisticGrowth(_growth_rate, K, nbMAL+nbFEM));
		return SimRunner::r.Poisson(beverton_hold(_growth_rate, K, nbMAL+nbFEM));
  }
  unsigned int setNbOffspring_Fecundity(const unsigned int& nbMAL, const unsigned int& nbFEM, const unsigned int& K){
		return my_round(nbFEM*_mean_fecundity);
  }
  unsigned int setNbOffspring_RandFecundity(const unsigned int& nbMAL, const unsigned int& nbFEM, const unsigned int& K){
		return SimRunner::r.Poisson(nbFEM*_mean_fecundity);
	}

  /**Link to the mating function, used to get the father from the mother in a Patch
    @param thePatch Patch instance of the current where the father has to be fetched
    @param mother index of the mother in the current Patch, used in the \a polyginy an \a monoginy mating systems
    @return the pointer to the father chosen following the mating scheme chosen
  **/
  virtual Individual* getMotherPtr (Patch* thePatch, unsigned int& index)
  { return (this->*getMother_func_ptr)(thePatch, index, FEM);  }
  virtual Individual* getFatherPtr (Patch* thePatch, unsigned int& index)
  { return (this->*getFather_func_ptr)(thePatch, index, _maleSex);  }

  ///@name getter
  ///@{

	string getMatingSystem_str () {
		switch(_mating_system){
			case 0: return "random mating (hermaphrodite)";
			case 1: return "selfing (hermaphrodite)";
			case 2: return "random mating (promiscuity)";
			case 3: return "polygyny";
			case 4: return "monogamy";
		}
		return "";
	}

	///@}

  ///@name Mating functions
  ///@{
  // get the individual of the index ///////////////////////////////////////////
  Individual* Index_MatingFunc                        (Patch*, unsigned int&, sex_t); // get the individual of the index

  // random mating /////////////////////////////////////////////////////////////
  Individual* Random_MatingFunc                       (Patch*, unsigned int&, sex_t);
  Individual* Random_Index_MatingFunc                 (Patch*, unsigned int&, sex_t); // return the index of the selected individual

  Individual* Random_S_MatingFunc                     (Patch*, unsigned int&, sex_t);
  Individual* Random_Index_S_MatingFunc               (Patch*, unsigned int&, sex_t);

  // full polygyny /////////////////////////////////////////////////////////////
  Individual* fullPolygyny_oneMale_MatingFunc         (Patch*, unsigned int&, sex_t);
  Individual* fullPolygyny_manyMales_MatingFunc       (Patch*, unsigned int&, sex_t);

  Individual* fullPolygyny_oneMale_S_MatingFunc       (Patch*, unsigned int&, sex_t); // the most fittest (not random)
  Individual* fullPolygyny_manyMales_S_MatingFunc     (Patch*, unsigned int&, sex_t); // get the x most fittest (not random)

  Individual* fullPolygyny_oneMale_S_MatingFunc2      (Patch*, unsigned int&, sex_t); // get the randomly chosen most fittest
  Individual* fullPolygyny_manyMales_S_MatingFunc2    (Patch*, unsigned int&, sex_t); // get the randomly chosen x most fittest

  // partial polygyny //////////////////////////////////////////////////////////
  Individual* partialPolygyny_oneMale_MatingFunc      (Patch*, unsigned int&, sex_t);
  Individual* partialPolygyny_manyMales_MatingFunc    (Patch*, unsigned int&, sex_t);

  Individual* partialPolygyny_oneMale_S_MatingFunc    (Patch*, unsigned int&, sex_t); // the most fittest (not random)
  Individual* partialPolygyny_manyMales_S_MatingFunc  (Patch*, unsigned int&, sex_t); // get the x most fittest (not random)

  Individual* partialPolygyny_oneMale_S_MatingFunc2   (Patch*, unsigned int&, sex_t); // get the randomly chosen most fittest
  Individual* partialPolygyny_manyMales_S_MatingFunc2 (Patch*, unsigned int&, sex_t); // get the randomly chosen x most fittest

  // monogamy //////////////////////////////////////////////////////////////////
  Individual* Monogyny_MatingFunc                     (Patch*, unsigned int&, sex_t); // mating pairs have to be first fixed
  Individual* Monogyny_S_MatingFunc                   (Patch*, unsigned int&, sex_t); // mating pairs have to be first fixed

  // one sex ///////////////////////////////////////////////////////////////////
  Individual* oneSex_notSameIndex_MatingFunc          (Patch*, unsigned int&, sex_t); // get random individual, but not the same index
  Individual* partialSelfing_MatingFunc               (Patch*, unsigned int&, sex_t); // partial sefling, the rest is random mating

  Individual* oneSex_notSameIndex_S_MatingFunc        (Patch*, unsigned int&, sex_t); // get random individual, but not the same index
  Individual* partialSelfing_S_MatingFunc             (Patch*, unsigned int&, sex_t); // partial sefling, the rest is random mating

  ///@}

  ///@name Implementations
  ///@{
  virtual bool init(Metapop* popPtr);
  virtual age_t removeAgeClass ( ) {return 0;}
  virtual age_t addAgeClass ( ) {return OFFSPRG;}
  virtual age_t requiredAgeClass () {return ADULTS;}
  ///@}

public:
	/** selection acts at the offsrping stage:
		* 1. a number of offspring is generated depending on the fecundity of the females (parameter "mean_fecundity")
		* 2. the number of surviving offspring is determined (parameter "mating_nb_offspring_model")
		* 3. downregulation ot his number is made depending ont he fitness of the offspring: teh fittest have a higher chance to survive
		*/
	void breed_selection_offspring_patch  ( );    // selection acts at the patch level (also used for neutral mating))
	void breed_selection_offspring_metapop( );    // selection acts at the metapop level (also used for neutral mating))
	void breed_selection_offspring_hard   ( );    // fitness is directly translated (also used for neutral mating))

	/** selection acts at the adults age: the higher the fitnes is the more offspring an individual has
		* 1. the nubmer of total offspring is determined (parameter "mating_nb_offspring_model")
		* 2. for each offspring parents are randomly drawn depending on their fitenss: the higher the parents fitness is the
		*    the more offsrping they get
		*/
	void breed_selection_neutral          ( );    // no selection acts
	void breed_selection_patch            ( );    // selection acts at the patch level
  void breed_selection_metapop          ( );    // selection acts at the metapop level
  void breed_selection_hard             ( );    // fitness is directly translated to nb_offspring

	 void reset_sex_after_phentoype(age_idx AGE);

  ///@name Implementations
  ///@{

  virtual void  execute ();
  
  virtual LCE_Breed* clone ( ) {return new LCE_Breed();}

  virtual void loadFileServices ( FileServices* loader ) {}
  virtual void loadStatServices ( StatServices* loader ) {}

	virtual void executeBeforeEachReplicate(const int& rep){}
	virtual void executeBeforeEachGeneration(const int& gen){
		if(gen == 1 && _threshold != my_NAN) reset_sex_after_phentoype(ADLTx); // that has to be done just before the start
	}
	///@}

};

#endif //LCEBREED_H


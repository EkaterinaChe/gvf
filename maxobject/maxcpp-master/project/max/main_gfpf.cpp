#include "maxcpp6.h"
#include "gfpf.h"
#include <string>
#include <map>
#include <vector>
#include <Eigen/Core>


using namespace std;


enum {STATE_CLEAR, STATE_LEARNING, STATE_FOLLOWING};

int restarted_l;
int restarted_d;
pair<float,float> xy0_l;
pair<float,float> xy0_d;
vector<float> vect_0_l;
vector<float> vect_0_d;


class Gfpfmax : public MaxCpp6<Gfpfmax> {
private:
	gfpf *bubi;
	int state;
	int lastreferencelearned;
	map<int,vector<pair<float,float> > > refmap;
	int Nspg, Rtpg;
	float sp, sv, sr, ss, so; // pos,vel,rot,scal,observation
	int pdim;
	Eigen::VectorXf mpvrs;
	Eigen::VectorXf rpvrs;
    
    
public:
    
//	Example(t_symbol * sym, long ac, t_atom * av) { 
//		setupIO(2, 2); // inlets / outlets
//	}
    Gfpfmax(t_symbol * sym, long argc, t_atom *argv)
    {
        setupIO(1, 6); // inlets / outlets
        post("Gfpfmax - realtime adaptive gesture recognition (11-04-2013)");
        post("(C) Baptiste Caramiaux, Ircam, Goldsmiths");
        
		// default values
		Nspg = 2000; int ns = Nspg; //!!
		Rtpg = 1000; int rt = Rtpg; //!!
		
		sp = 0.0001;
		sv = 0.01;
		ss = 0.0001;
		sr = 0.000001;
		
		
		so = 0.2;
		
		pdim = 4;
		Eigen::VectorXf sigs(pdim);
		sigs << sp, sv, ss, sr;
		
		bubi = new gfpf(ns, sigs, 1./(so * so), rt, 0.);
		
		
		mpvrs = Eigen::VectorXf(pdim);
		rpvrs = Eigen::VectorXf(pdim);
		mpvrs << 0.05, 1.0, 1.0, 0.0;
		rpvrs << 0.1,  0.4, 0.3, 0.0;
		
		restarted_l=1;
		restarted_d=1;
				
		state = STATE_CLEAR;
		
		lastreferencelearned = -1;
		
    }

    ~Gfpfmax()
    {
        post("destroying Gfpfmax object");
		if(bubi != NULL)
			delete bubi;
    }

	
	// methods:
	void bang(long inlet) {
		outlet_bang(m_outlets[0]);
	}
	void testfloat(long inlet, double v) {
                        post("shit float");
		outlet_float(m_outlets[0], v);
	}
	void testint(long inlet, long v) {
                        post("shit long");
		outlet_int(m_outlets[0], v);
	}
	void learn(long inlet, t_symbol * s, long ac, t_atom * av) {
//                        post("just shit %s %d %f", sym.c_str(),ac, atom_getfloat(&av[0]));
			
			if(ac != 1)
			{
				post("wrong number of argument (must be 1)");
				return;
			}
			int refI = atom_getlong(&av[0]);
			if(refI != lastreferencelearned+1)
			{
				post("you need to learn reference %d first",lastreferencelearned+1);
				return;
			}
			lastreferencelearned++;
			refmap[refI] = vector<pair<float, float> >();
			post("learning reference %d", refI);
			
			bubi->addTemplate();
			
			state = STATE_LEARNING;
			
			restarted_l=1;
			
    }
    	void follow(long inlet, t_symbol * s, long ac, t_atom * av) {

			if(lastreferencelearned >= 0)
			{
				post("I'm about to follow!");
				
				bubi->spreadParticles(mpvrs,rpvrs);
				//post("nb of gest after following %i", bubi->getNbOfGestures());
				
				state = STATE_FOLLOWING;
				
			}
			else
			{
				post("no reference has been learned");
				return;
			}
        }
    void clear(long inlet, t_symbol * s, long ac, t_atom * av) {
			lastreferencelearned = -1;
			
			bubi->clear();
			
			restarted_l=1;
			restarted_d=1;
			
			state = STATE_CLEAR;
		}

    void data(long inlet, t_symbol * s, long ac, t_atom * av) {
			if(state == STATE_CLEAR)
			{
				post("what am I supposed to do ... I'm in standby!");
				return;
			}
			if(ac == 0)
			{
				post("invalid format, points have at least 1 coordinate");
				return;
			}
			if(state == STATE_LEARNING)
			{
                vector<float> vect(ac);
                if (ac ==2){
                    if (restarted_l==1)
                    {
                        // store the incoming list as a vector of float
                        for (int k=0; k<ac; k++){
                            vect[k] = atom_getfloat(&av[k]);
                        }
                        
                        // keep track of the first point
                        vect_0_l = vect;
                        restarted_l=0;
                    }
                    for (int k=0; k<ac; k++){
                        vect[k] = atom_getfloat(&av[k]);
                        vect[k]=vect[k]-vect_0_l[k];
                    }
                }
                else {
                    for (int k=0; k<ac; k++)
                        vect[k] = atom_getfloat(&av[k]);
                }
                
                //				pair<float,float> xy;
                //				xy.first  = x -xy0_l.first;
                //				xy.second = y -xy0_l.second;
                
				// Fill template
				bubi->fillTemplate(lastreferencelearned,vect);
				
				
			}
			else if(state == STATE_FOLLOWING)
			{
                vector<float> vect(ac);
                if (ac==2){
                    if (restarted_d==1)
                    {
                        // store the incoming list as a vector of float
                        for (int k=0; k<ac; k++){
                            vect[k] = atom_getfloat(&av[k]);
                        }
                        
                        // keep track of the first point
                        vect_0_d = vect;
                        restarted_d=0;
                    }
                    
                    for (int k=0; k<ac; k++){
                        vect[k] = atom_getfloat(&av[k]);
                        vect[k]=vect[k]-vect_0_d[k];
                    }
                }
                else{
                    post("%i",ac);
                    for (int k=0; k<ac; k++)
                        vect[k] = atom_getfloat(&av[k]);
                }
				
				//post("%f %f",xy(0,0),xy(0,1));
				// ------- Fill template
				bubi->infer(vect);
				
				
				
				// output recognition
				
				
				
				Eigen::MatrixXf statu = bubi->getEstimatedStatus(); //getGestureProbabilities();
				
				t_atom *outAtoms = new t_atom[statu.rows()];
				for(int j = 0; j < statu.rows(); j++)
					atom_setfloat(&outAtoms[j],statu(j,0));
                outlet_anything(m_outlets[0], gensym("test1"), statu.rows(), outAtoms);
				delete[] outAtoms;
				
				outAtoms = new t_atom[statu.rows()];
				for(int j = 0; j < statu.rows(); j++)
					atom_setfloat(&outAtoms[j],statu(j,1));
                outlet_anything(m_outlets[1], gensym("test2"), statu.rows(), outAtoms);
				delete[] outAtoms;
				
				outAtoms = new t_atom[statu.rows()];
				for(int j = 0; j < statu.rows(); j++)
					atom_setfloat(&outAtoms[j],statu(j,2));
                outlet_anything(m_outlets[2], gensym("test3"), statu.rows(), outAtoms);
				delete[] outAtoms;
				
				outAtoms = new t_atom[statu.rows()];
				for(int j = 0; j < statu.rows(); j++)
					atom_setfloat(&outAtoms[j],statu(j,3));
                outlet_anything(m_outlets[3], gensym("test4"), statu.rows(), outAtoms);
				delete[] outAtoms;
				
				Eigen::VectorXf gprob = bubi->getGestureConditionnalProbabilities();
				outAtoms = new t_atom[gprob.size()];
				for(int j = 0; j < gprob.size(); j++)
					atom_setfloat(&outAtoms[j],statu(j,4));
                outlet_anything(m_outlets[4], gensym("test5"), statu.rows(), outAtoms);
				delete[] outAtoms;
                
                gprob = bubi->getGestureLikelihoods();
				outAtoms = new t_atom[gprob.size()];
				for(int j = 0; j < gprob.size(); j++)
					atom_setfloat(&outAtoms[j],statu(j,5));
                outlet_anything(m_outlets[5], gensym("test6"), statu.rows(), outAtoms);
				delete[] outAtoms;
				
				
				
			}
		}
    void printme(long inlet, t_symbol * s, long ac, t_atom * av) {
            post("N. particles %d: ", bubi->getNbOfParticles());
            post("Resampling Th. %d: ", bubi->getResamplingThreshold());
            post("Means %.3f %.3f %.3f %.3f: ", mpvrs[0], mpvrs[1], mpvrs[2], mpvrs[3]);
            post("Ranges %.3f %.3f %.3f %.3f: ", rpvrs[0], rpvrs[1], rpvrs[2], rpvrs[3]);
			for(int i = 0; i < bubi->getNbOfTemplates(); i++)
			{
				post("reference %d: ", i);
				vector<vector<float> > tplt = bubi->getTemplateByInd(i);
				for(int j = 0; j < tplt.size(); j++)
				{
					post("%02.4f  %02.4f", tplt[j][0], tplt[j][1]);
				}
			}
			
		}
    void restart(long inlet, t_symbol * s, long ac, t_atom * av) {
			restarted_l=1;
			if(state == STATE_FOLLOWING)
			{
				
				bubi->spreadParticles(mpvrs,rpvrs);
				
				restarted_l=1;
				restarted_d=1;
			}
		}
    void std(long inlet, t_symbol * s, long ac, t_atom * av) {
			float stdnew = atom_getfloat(&av[0]);
			if (stdnew == 0.0)
				stdnew = 0.1;
			bubi->setIcovSingleValue(1/(stdnew*stdnew));
		}
    void rt(long inlet, t_symbol * s, long ac, t_atom * av) {
			int rtnew = atom_getlong(&av[0]);
            int cNS = bubi->getNbOfParticles();
			if (rtnew >= cNS)
				rtnew = floor(cNS/2);
			bubi->setResamplingThreshold(rtnew);
		}
    void means(long inlet, t_symbol * s, long ac, t_atom * av) {
            mpvrs = Eigen::VectorXf(pdim);
            mpvrs << atom_getfloat(&av[0]), atom_getfloat(&av[1]), atom_getfloat(&av[2]), atom_getfloat(&av[3]);
        }
    void ranges(long inlet, t_symbol * s, long ac, t_atom * av) {
            rpvrs = Eigen::VectorXf(pdim);
            rpvrs << atom_getfloat(&av[0]), atom_getfloat(&av[1]), atom_getfloat(&av[2]), atom_getfloat(&av[3]);
        }
    void adaptSpeed(long inlet, t_symbol * s, long ac, t_atom * av) {
			vector<float> as;
			as.push_back(atom_getfloat(&av[0]));
			as.push_back(atom_getfloat(&av[1]));
			as.push_back(atom_getfloat(&av[2]));
			as.push_back(atom_getfloat(&av[3]));
            
			bubi->setAdaptSpeed(as);
		}
	
};


 //THIS IS FOR Max6.1

//C74_EXPORT extern "C" int main(void) {
//	// create a class with the given name:
//	Gfpfmax::makeMaxClass("gfpfmax");
//	REGISTER_METHOD(Gfpfmax, bang);
//	REGISTER_METHOD_FLOAT(Gfpfmax, testfloat);
//	REGISTER_METHOD_LONG(Gfpfmax, testint);
//	REGISTER_METHOD_GIMME(Gfpfmax, test);
//}


extern "C" int main(void) {
     // create a class with the given name:
    Gfpfmax::makeMaxClass("gfpfmax");
    REGISTER_METHOD(Gfpfmax, bang);
    REGISTER_METHOD_FLOAT(Gfpfmax, testfloat);
    REGISTER_METHOD_LONG(Gfpfmax,  testint);
    REGISTER_METHOD_GIMME(Gfpfmax, learn);
    REGISTER_METHOD_GIMME(Gfpfmax, follow);
    REGISTER_METHOD_GIMME(Gfpfmax, clear);
    REGISTER_METHOD_GIMME(Gfpfmax, data);
    REGISTER_METHOD_GIMME(Gfpfmax, printme);
    REGISTER_METHOD_GIMME(Gfpfmax, restart);
    REGISTER_METHOD_GIMME(Gfpfmax, std);
    REGISTER_METHOD_GIMME(Gfpfmax, rt);
    REGISTER_METHOD_GIMME(Gfpfmax, means);
    REGISTER_METHOD_GIMME(Gfpfmax, ranges);
    REGISTER_METHOD_GIMME(Gfpfmax, adaptSpeed);
}
#ifndef BODY_H_
#define BODY_H_

#include "elementary.h"

namespace bc{
enum BoundaryCondition {slip, noslip, kutta, noperturbations, tricky};}
namespace sc{
enum SourceCondition {none, source, sink};}
namespace hc{
enum HeatCondition {neglect, isolate, temperature, power};}

class TBody;
class TAtt : public TVec
{
	public:
		double g, q;
		double gsum;
		double pres, fric;
		double heat, heat_const;
		TVec dl;
		bc::BoundaryCondition bc;
		hc::HeatCondition hc;

		TAtt(){}
		//TAtt(TBody *body, int eq_no);
		void zero() { rx = ry = g = q = pres = fric = gsum = heat = 0; }
		TAtt& operator= (const TVec& p) { rx=p.rx; ry=p.ry; return *this; }

	public:
		int eq_no;
		TBody* body;
};

class TBody
{
	public:
		TBody();
		~TBody();

		//int LoadFromFile(const char* filename, int start_eq_no);
		void Rotate(double angle);
		TAtt* PointIsInvalid(TVec p);
		double SurfaceLength();
		double size(){ return List->size_safe(); }
		void SetRotation(TVec sRotAxis, double (*sRotSpeed)(double time), double sRotSpeed_const = 0);

		vector<TObj> *List;
		vector<TAtt> *AttachList;
		bool InsideIsValid;
		bool isInsideValid();
		double RotSpeed(double time) const
			{ return RotSpeed_link?RotSpeed_link(time):RotSpeed_const; }
		double Angle;
		TVec Position;
		TVec RotAxis;
		void UpdateAttach();

		void calc_pressure(); // need /dt
		//void calc_friction(); // need /Pi/Eps_min^2
		void zero_variables();

		TObj Force; //dont forget to zero it when u want
		double g_dead;

		TObj* next(TObj* obj) const { return List->next(obj); }
		TObj* prev(TObj* obj) const { return List->prev(obj); }
		TAtt* next(TAtt* att) const { return AttachList->next(att); }
		TAtt* prev(TAtt* att) const { return AttachList->prev(att); }
		TAtt* att(const TObj* obj) const { return &AttachList->at(obj - List->begin());}
		TObj* obj(const TAtt* att) const { return &List->at(att - AttachList->begin());}

		//Heat layer
		//void CleanHeatLayer();
		//int *ObjectIsInHeatLayer(TObj &obj); //link to element of HeatLayer array

	private:
		double (*RotSpeed_link)(double time);
		double RotSpeed_const;
};

#endif /* BODY_H_ */

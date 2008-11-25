// -*- C++ -*-
//
// Package:    DeDxDiscriminatorProducer
// Class:      DeDxDiscriminatorProducer
// 
/**\class DeDxDiscriminatorProducer DeDxDiscriminatorProducer.cc RecoTracker/DeDxDiscriminatorProducer/src/DeDxDiscriminatorProducer.cc

 Description: <one line class summary>

 Implementation:
     <Notes on implementation>
*/
//
// Original Author:  andrea
//         Created:  Thu May 31 14:09:02 CEST 2007
//    Code Updates:  loic Quertenmont (querten)
//         Created:  Thu May 10 14:09:02 CEST 2008
// $Id: DeDxDiscriminatorProducer.cc,v 1.3 2008/09/15 09:43:12 querten Exp $
//
//


// system include files
#include <memory>
#include "DataFormats/Common/interface/ValueMap.h"

#include "RecoTracker/DeDx/interface/DeDxDiscriminatorProducer.h"
#include "DataFormats/TrackReco/interface/DeDxData.h"
//#include "DataFormats/TrackReco/interface/TrackDeDxHits.h"
//#include "DataFormats/TrackReco/interface/DeDxHit.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "TrackingTools/PatternTools/interface/TrajTrackAssociation.h"

#include "RecoTracker/DeDx/interface/GenericAverageDeDxEstimator.h"
#include "RecoTracker/DeDx/interface/TruncatedAverageDeDxEstimator.h"
#include "RecoTracker/DeDx/interface/MedianDeDxEstimator.h"

#include "FWCore/Framework/interface/ESHandle.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"


#include "CondFormats/DataRecord/interface/SiStripDeDxMipRcd.h"


using namespace reco;
using namespace std;
using namespace edm;

DeDxDiscriminatorProducer::DeDxDiscriminatorProducer(const edm::ParameterSet& iConfig)
{

   produces<ValueMap<DeDxData> >();

   m_tracksTag = iConfig.getParameter<edm::InputTag>("tracks");
   m_trajTrackAssociationTag   = iConfig.getParameter<edm::InputTag>("trajectoryTrackAssociation");

   usePixel = iConfig.getParameter<bool>("UsePixel"); 
   useStrip = iConfig.getParameter<bool>("UseStrip");
   if(!usePixel && !useStrip)
   edm::LogWarning("DeDxHitsProducer") << "Pixel Hits AND Strip Hits will not be used to estimate dEdx --> BUG, Please Update the config file";

   DiscriminatorMode   = iConfig.getUntrackedParameter<bool>("DiscriminatorMode", true);
   MapFileName         = iConfig.getParameter<std::string>("MapFile");
   Formula             = iConfig.getUntrackedParameter<unsigned>("Formula"            ,  0);

   MinTrackMomentum    = iConfig.getUntrackedParameter<double>  ("minTrackMomentum"   ,  3.0);
   MaxTrackMomentum    = iConfig.getUntrackedParameter<double>  ("maxTrackMomentum"   ,  99999.0); 
   MinTrackEta         = iConfig.getUntrackedParameter<double>  ("minTrackEta"        , -5.0);
   MaxTrackEta         = iConfig.getUntrackedParameter<double>  ("maxTrackEta"        ,  5.0);
   MaxNrStrips         = iConfig.getUntrackedParameter<unsigned>("maxNrStrips"        ,  2);
   MinTrackHits        = iConfig.getUntrackedParameter<unsigned>("MinTrackHits"       ,  8);
   AllowSaturation     = iConfig.getUntrackedParameter<bool>    ("AllowSaturation"    ,  false);
}


DeDxDiscriminatorProducer::~DeDxDiscriminatorProducer(){}

// ------------ method called once each job just before starting event loop  ------------
void  DeDxDiscriminatorProducer::beginJob(const edm::EventSetup& iSetup){

   cout << "TEST1\n";


   edm::ESHandle<PhysicsTools::Calibration::HistogramD2D> DeDxMapHandle_;    
   iSetup.get<SiStripDeDxMipRcd>().get(DeDxMapHandle_);
   DeDxMap_ = *DeDxMapHandle_.product();

   //   iSetup_                  = &iSetup;

   cout << "TEST2\n";


   MapFile                   = new TFile(MapFileName.c_str(), "RECREATE");
/*      Charge_Vs_Path_Barrel     = new TH2F ("Charge_Vs_Path_Barrel"     , "Charge_Vs_Path_Barrel" , 250, 0.2, 1.4, 1000, 0, 5000);
      Charge_Vs_Path_Endcap     = new TH2F ("Charge_Vs_Path_Endcap"     , "Charge_Vs_Path_Endcap" , 250, 0.2, 1.4, 1000, 0, 5000);

      MapFile                   = new TFile(MapFileName.c_str());

      Charge_Vs_Path_Barrel     = (TH2F*) MapFile->FindObjectAny("Charge_Vs_Path_Barrel");
      Charge_Vs_Path_Endcap     = (TH2F*) MapFile->FindObjectAny("Charge_Vs_Path_Endcap");
*/


      PCharge_Vs_Path_Barrel    = new TH2F ("PCharge_Vs_Path_Barrel"     , "PCharge_Vs_Path_Barrel" , DeDxMap_.numberOfBinsX(), 0.2, 1.4, DeDxMap_.numberOfBinsY() , 0, 5000);
      PCharge_Vs_Path_Endcap    = new TH2F ("PCharge_Vs_Path_Endcap"     , "PCharge_Vs_Path_Endcap" , 250, 0.2, 1.4, 1000, 0, 5000);

   cout << "TEST3\n";

   
   for(int i=0;i<PCharge_Vs_Path_Barrel->GetXaxis()->GetNbins();i++){
      for(int j=0;j<PCharge_Vs_Path_Barrel->GetYaxis()->GetNbins();j++){
//         printf("%06i/%06i  & %06i/%06i\n",i,PCharge_Vs_Path_Barrel->GetXaxis()->GetNbins(), j, PCharge_Vs_Path_Barrel->GetYaxis()->GetNbins());
         float tmp1 = DeDxMap_.binContent(i,j);
	 //         float tmp2 = DeDxMap_.binError  (i,j);
	 //   printf("%3i-%3i --> %6f - %6f\n",i,j,tmp1, tmp2);
         PCharge_Vs_Path_Barrel->SetBinContent (i, j, tmp1);
	 //         PCharge_Vs_Path_Barrel->SetBinError   (i, j, tmp2);
      }
   }

   edm::ESHandle<TrackerGeometry> tkGeom;
   iSetup.get<TrackerDigiGeometryRecord>().get( tkGeom );
   //   m_tracker=&(* tkGeom );

   cout << "TEST4\n";


   vector<GeomDet*> Det = tkGeom->dets();
   for(unsigned int i=0;i<Det.size();i++){
      DetId  Detid  = Det[i]->geographicalId();
      int    SubDet = Detid.subdetId();

      if( SubDet == StripSubdetector::TIB ||  SubDet == StripSubdetector::TID ||
          SubDet == StripSubdetector::TOB ||  SubDet == StripSubdetector::TEC  ){

          StripGeomDetUnit* DetUnit     = dynamic_cast<StripGeomDetUnit*> (Det[i]);
          if(!DetUnit)continue;

          const StripTopology& Topo     = DetUnit->specificTopology();
          unsigned int         NAPV     = Topo.nstrips()/128;

          double Eta     = DetUnit->position().basicVector().eta();
          double R       = DetUnit->position().basicVector().transverse();
          double Thick   = DetUnit->surface().bounds().thickness();

          stModInfo* MOD = new stModInfo;
          MOD->DetId     = Detid.rawId();
          MOD->SubDet    = SubDet;
          MOD->Eta       = Eta;
          MOD->R         = R;
          MOD->Thickness = Thick;
          MOD->NAPV      = NAPV;
          MODsColl[MOD->DetId] = MOD;
      }
   }
 
   cout << "TEST5\n";

}

// ------------ method called once each job just after ending the event loop  ------------
void  DeDxDiscriminatorProducer::endJob(){
   cout << "TEST6\n";

   PCharge_Vs_Path_Barrel->Write();

   MapFile->Write();
   MapFile->Close();
}



void DeDxDiscriminatorProducer::produce(edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  return;
/*
  iEvent_ = &iEvent;  

  auto_ptr<ValueMap<DeDxData> > trackDeDxDiscrimAssociation(new ValueMap<DeDxData> );  
  ValueMap<DeDxData>::Filler filler(*trackDeDxDiscrimAssociation);

  Handle<TrajTrackAssociationCollection> trajTrackAssociationHandle;
  iEvent.getByLabel(m_trajTrackAssociationTag, trajTrackAssociationHandle);
  const TrajTrackAssociationCollection TrajToTrackMap = *trajTrackAssociationHandle.product();

  edm::Handle<reco::TrackCollection> trackCollectionHandle;
  iEvent.getByLabel(m_tracksTag,trackCollectionHandle);
 
   std::vector<DeDxData> dEdxDiscrims( TrajToTrackMap.size() );


   cout << "DDP  TEST1\n";


   unsigned track_index = 0;
   for(TrajTrackAssociationCollection::const_iterator it = TrajToTrackMap.begin(); it!=TrajToTrackMap.end(); ++it, track_index++) {
      dEdxDiscrims[track_index] = DeDxData(-1, -1, 0 );

      const Track      track = *it->val;
      const Trajectory traj  = *it->key;

      if(track.eta()<MinTrackEta || track.eta()>MaxTrackEta){printf("Eta Cut\n");continue;}
      if(track.p()<MinTrackMomentum || track.p()>MaxTrackMomentum){printf("Pt Cut\n");continue;}
      if(track.found()<MinTrackHits){printf("Hits Cut\n");continue;}


      vector<TrajectoryMeasurement> measurements = traj.measurements();
      if(traj.foundHits()<(int)MinTrackHits)continue;

      MeasurementProbabilities.clear();
      for(vector<TrajectoryMeasurement>::const_iterator measurement_it = measurements.begin(); measurement_it!=measurements.end(); measurement_it++){

         TrajectoryStateOnSurface trajState = measurement_it->updatedState();
         if( !trajState.isValid() ) continue;

         const TrackingRecHit*         hit               = (*measurement_it->recHit()).hit();
         const SiStripRecHit2D*        sistripsimplehit  = dynamic_cast<const SiStripRecHit2D*>(hit);
         const SiStripMatchedRecHit2D* sistripmatchedhit = dynamic_cast<const SiStripMatchedRecHit2D*>(hit);

         if(sistripsimplehit)
         {
             ComputeChargeOverPath(sistripsimplehit, trajState, &iSetup, &track, traj.chiSquared());
         }else if(sistripmatchedhit){
             ComputeChargeOverPath(sistripmatchedhit->monoHit()  ,trajState, &iSetup, &track, traj.chiSquared());
             ComputeChargeOverPath(sistripmatchedhit->stereoHit(),trajState, &iSetup, &track, traj.chiSquared());
         }else{
         }
      }


      if(DiscriminatorMode){
         int size = MeasurementProbabilities.size();

         double estimator = 1;
         if(Formula==0){
            double P = 1;
            for(int i=0;i<size;i++){
               if(MeasurementProbabilities[i]<=0.001){P *= pow(0.001f, 1.0f/size);}
               else                                  {P *= pow(MeasurementProbabilities[i], 1.0f/size);}
            }
            estimator = P;
         }else if(Formula==1){

            if(MeasurementProbabilities.size()>0){

              std::sort(MeasurementProbabilities.begin(), MeasurementProbabilities.end(), std::less<double>() );
              for(int i=0;i<size;i++){if(MeasurementProbabilities[i]<=0.001)MeasurementProbabilities[i] = 0.001f;    }

               double SumJet = 0.;
               for(int i=0;i<size;i++){ SumJet+= log(MeasurementProbabilities[i]); }

              double Loginvlog=log(-SumJet);
              double Prob =1.;
              double lfact=1.;

              for(int l=1; l!=size; l++){
                   lfact*=l;
                   Prob+=exp(l*Loginvlog-log(1.*lfact));
               }

               double LogProb=log(Prob);
               double ProbJet=std::min(exp(std::max(LogProb+SumJet,-30.)),1.);
               estimator = -log10(ProbJet)/4.;
               estimator = 1-estimator;
            }else{
               estimator = -1;
            }
         }else if(Formula==2){
           estimator = -2;
           if(size>0){
               std::sort(MeasurementProbabilities.begin(), MeasurementProbabilities.end(), std::less<double>() );
               double P = 1.0/(12*size);
               for(int i=1;i<=size;i++){
                  P += pow(MeasurementProbabilities[i-1] - ((2.0*i-1.0)/(2.0*size)),2);
               }
               P *= (1.0/size);
               estimator = P;
               if(estimator>=0.333)printf("BUG\n");
            }
         }else{
           estimator = -2;
           if(size>0){
               std::sort(MeasurementProbabilities.begin(), MeasurementProbabilities.end(), std::less<double>() );
               double P = 1.0/(12*size);
               for(int i=1;i<=size;i++){
                  P += MeasurementProbabilities[i-1] * pow(MeasurementProbabilities[i-1] - ((2.0*i-1.0)/(2.0*size)),2);
               }
               P *= (1.0/size);
               estimator = P;
               if(estimator>=0.333)printf("BUG\n");
           }
         }

         dEdxDiscrims[track_index] = DeDxData(estimator, -1, size );
      }
   }

  filler.insert(trackCollectionHandle, dEdxDiscrims.begin(), dEdxDiscrims.end());
  filler.fill();
  iEvent.put(trackDeDxDiscrimAssociation);
  */
}


double
DeDxDiscriminatorProducer::ComputeChargeOverPath(const SiStripRecHit2D* sistripsimplehit,TrajectoryStateOnSurface trajState, const edm::EventSetup* iSetup,  const Track* track, double trajChi2OverN)
{

   LocalVector          trackDirection = trajState.localDirection();
   double                  cosine      = trackDirection.z()/trackDirection.mag();
   const SiStripCluster*   Cluster     = (sistripsimplehit->cluster()).get();
   const vector<uint8_t>&  Ampls       = Cluster->amplitudes();
   uint32_t                DetId       = Cluster->geographicalId();
   int                     FirstStrip  = Cluster->firstStrip();
   bool                    Saturation  = false;
   bool                    Overlaping  = false;
   int                     Charge      = 0;
   stModInfo* MOD                      = MODsColl[DetId];


   if(!IsFarFromBorder(trajState,DetId, iSetup)){/*printf("tooCloseFromBorder\n");*/return -1;}


   if(FirstStrip==0                                  )Overlaping=true;
   if(FirstStrip==128                                )Overlaping=true;
   if(FirstStrip==256                                )Overlaping=true;
   if(FirstStrip==384                                )Overlaping=true;
   if(FirstStrip==512                                )Overlaping=true;
   if(FirstStrip==640                                )Overlaping=true;

   if(FirstStrip<=127 && FirstStrip+Ampls.size()>127)Overlaping=true;
   if(FirstStrip<=255 && FirstStrip+Ampls.size()>255)Overlaping=true;
   if(FirstStrip<=383 && FirstStrip+Ampls.size()>383)Overlaping=true;
   if(FirstStrip<=511 && FirstStrip+Ampls.size()>511)Overlaping=true;
   if(FirstStrip<=639 && FirstStrip+Ampls.size()>639)Overlaping=true;

   if(FirstStrip+Ampls.size()==127                   )Overlaping=true;
   if(FirstStrip+Ampls.size()==255                   )Overlaping=true;
   if(FirstStrip+Ampls.size()==383                   )Overlaping=true;
   if(FirstStrip+Ampls.size()==511                   )Overlaping=true;
   if(FirstStrip+Ampls.size()==639                   )Overlaping=true;
   if(FirstStrip+Ampls.size()==767                   )Overlaping=true;
//   if(!DiscriminatorMode && Overlaping){printf("Overlapping\n");return -1;}


   for(unsigned int a=0;a<Ampls.size();a++){Charge+=Ampls[a];if(Ampls[a]>=254)Saturation=true;}
   double path                    = (10.0*MOD->Thickness)/fabs(cosine);
   double ClusterChargeOverPath   = (double)Charge / path ;

   if(Ampls.size()>MaxNrStrips)      {/*printf("tooMuchStrips\n");*/return -1;}
//   if(!DiscriminatorMode && Saturation && !AllowSaturation){printf("Saturation\n");return -1;}

   if(!DiscriminatorMode){
      if(MOD->SubDet == StripSubdetector::TIB || MOD->SubDet == StripSubdetector::TOB) Charge_Vs_Path_Barrel->Fill(path,ClusterChargeOverPath);
      if(MOD->SubDet == StripSubdetector::TID || MOD->SubDet == StripSubdetector::TEC) Charge_Vs_Path_Endcap->Fill(path,ClusterChargeOverPath);
   }else{
      TH2F* MapToUse = NULL;
      if(MOD->SubDet == StripSubdetector::TIB || MOD->SubDet == StripSubdetector::TOB) MapToUse = PCharge_Vs_Path_Barrel;
      if(MOD->SubDet == StripSubdetector::TID || MOD->SubDet == StripSubdetector::TEC) MapToUse = PCharge_Vs_Path_Endcap;

      int   BinX  = MapToUse->GetXaxis()->FindBin(path);
      int   BinY  = MapToUse->GetYaxis()->FindBin(ClusterChargeOverPath);

      float Prob = MapToUse->GetBinContent(BinX,BinY);

      if(Prob>=0)MeasurementProbabilities.push_back(Prob);
   }

   return ClusterChargeOverPath;
}


bool DeDxDiscriminatorProducer::IsFarFromBorder(TrajectoryStateOnSurface trajState, const uint32_t detid, const edm::EventSetup* iSetup)
{
  edm::ESHandle<TrackerGeometry> tkGeom; iSetup->get<TrackerDigiGeometryRecord>().get( tkGeom );

  LocalPoint  HitLocalPos   = trajState.localPosition();
  LocalError  HitLocalError = trajState.localError().positionError() ;

  const GeomDetUnit* it = tkGeom->idToDetUnit(DetId(detid));
  if (dynamic_cast<const StripGeomDetUnit*>(it)==0 && dynamic_cast<const PixelGeomDetUnit*>(it)==0) {
     std::cout << "this detID doesn't seem to belong to the Tracker" << std::endl;
     return false;
  }

  const BoundPlane plane = it->surface();
  const TrapezoidalPlaneBounds* trapezoidalBounds( dynamic_cast<const TrapezoidalPlaneBounds*>(&(plane.bounds())));
  const RectangularPlaneBounds* rectangularBounds( dynamic_cast<const RectangularPlaneBounds*>(&(plane.bounds())));

  double DistFromBorder = 1.0;
  double HalfWidth      = it->surface().bounds().width()  /2.0;
  double HalfLength     = it->surface().bounds().length() /2.0;

  if(trapezoidalBounds)
  {
     std::vector<float> const & parameters = (*trapezoidalBounds).parameters();
     HalfLength     = parameters[3];
     double t       = (HalfLength + HitLocalPos.y()) / (2*HalfLength) ;
     HalfWidth      = parameters[0] + (parameters[1]-parameters[0]) * t;
  }else if(rectangularBounds){
     HalfWidth      = it->surface().bounds().width()  /2.0;
     HalfLength     = it->surface().bounds().length() /2.0;
  }else{return false;}

//  if (fabs(HitLocalPos.x())+HitLocalError.xx() >= (HalfWidth  - DistFromBorder) ) return false;//Don't think is really necessary
  if (fabs(HitLocalPos.y())+HitLocalError.yy() >= (HalfLength - DistFromBorder) ) return false;

  return true;
}



//define this as a plug-in
DEFINE_FWK_MODULE(DeDxDiscriminatorProducer);

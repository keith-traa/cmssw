#include "FWCore/Framework/interface/MakerMacros.h"

#include "RecoTracker/TkTrackingRegions/interface/TrackingRegionProducerFactory.h"
#include "RecoTracker/TkTrackingRegions/interface/TrackingRegionEDProducerT.h"
#include "RecoEgamma/EgammaElectronProducers/plugins/TrackingRegionsFromSuperClustersProducer.h"
DEFINE_EDM_PLUGIN(TrackingRegionProducerFactory,
                  TrackingRegionsFromSuperClustersProducer,
                  "TrackingRegionsFromSuperClustersProducer");
using TrackingRegionsFromSuperClustersEDProducer = TrackingRegionEDProducerT<TrackingRegionsFromSuperClustersProducer>;
DEFINE_FWK_MODULE(TrackingRegionsFromSuperClustersEDProducer);

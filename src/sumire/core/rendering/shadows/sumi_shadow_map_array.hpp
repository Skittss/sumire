#pragma once

/*
* 
* TODO: Implementation of shadow maps as a fixed-dimensions texture array.
* 
* Less flexbile than using a VSM or shadow atlas, but far more simple to implement and use.
* 
* A potential solution for flexibility is to make multiple of these texture arrays for different levels of shadow detail.
* 
*/

namespace sumire {

    class SumiShadowMapArray {
    public:
        SumiShadowMapArray();
    private:
    };

}
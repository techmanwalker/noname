#pragma once

class CoverRef;

namespace covers::live {

class cover_provider {

public:
    virtual ~cover_provider () = default;

    /** 
        @brief register a given generated cover reference to this cover provider
    
        @details make the cover_provider aware that this cover exists at all
        so it can properly generate, cache and retrieve your tracks and albums'
        covers and thumbnails fast enough to keep your player responsive
    */
    virtual void register_cover_reference (const CoverRef &ref) = 0;

};

}
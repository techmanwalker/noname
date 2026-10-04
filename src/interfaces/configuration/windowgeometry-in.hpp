#pragma once

class WindowGeometry
{

public:
    virtual ~WindowGeometry () = default;    

    virtual int width  () const = 0;
    virtual int height () const = 0;

    virtual void poll_width  (int width)  = 0;
    virtual void poll_height (int height) = 0;

    virtual void poll_and_save_to_disk (int width, int height) = 0;
};
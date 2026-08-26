#pragma once

class WindowGeometry {

public:
    virtual ~WindowGeometry () = default;    

    virtual int width() const = 0;
    virtual int height() const = 0;

    virtual void save(int width, int height) = 0;
};
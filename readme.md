This block wraps the nuitrack SDK for body tracking. It comes with an example Boxman which draws all the bones in the orientation and location reported by the SDK.

# WARNING
Quick word of warning up front: I am experiencing mysterious crashes with the nuitrack SDK that I'm not able to track down since the SDK is closed source. As far as I can see, if a single user is being tracked for a longer period of time (minutes) at some point the skeleton data will freeze. As soon as that user steps out of the tracking frame the nuitrack SDK crashes and burns. I got an opencv error message, something to do with resizing an empty frame I believe, but since the SDK is closed source I figured it wasn't worth investigating further.

More ranting [here](https://3dclub.orbbec3d.com/t/body-tracking-not-free-after-january-31-2018/1273/24?u=dvh)

# UPDATE
After being in touch with the nuitrack team it seems like buying and activating a license key will solve the crashes I experienced. In a future update a proper license exception will be thrown instead. I did not have the opportunity to test this yet.

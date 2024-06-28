local morePlanetInfoPatch = [===[
[
    {
        "op": "test",
        "path": "/weatherThreatValues"
    },
    {
        "op": "add",
        "path": "/weatherThreatValues/clear",
        "value": 0.0
    }
]
]===]
-- The patch must be done post-load to «catch» the existence of More Planet Info.
assets.add("/interface/cockpit/cockpit.config.postproc", morePlanetInfoPatch)
assets.patch("/interface/cockpit/cockpit.config", "/interface/cockpit/cockpit.config.postproc")
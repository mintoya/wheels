const Vason = require("./vason.js");
const exampleString = `
{ 
    keyboard:{
      tapdances:{
        { taps:{ K:A }, holds:{M:la , M:ls }, }
        { taps:{ K:B }, holds:{M:lc , M:rs }, }
      },
      layers:{ 
        { 
          K:ESC,  K:Q,  K:W, K:E,   K:R, K:T,        K:Y, K:U,   K:I,  K:O,   K:P,  K:"[",
           M:ls,  K:A,  K:S, K:D,   K:F, K:G,        K:H, K:J,   K:K,  K:L, K:";",  K:"'", 
           M:lc,  K:Z,  K:X, K:C,   K:V, K:B,        K:N, K:M, K:",",  K:.,  K:\/,  K:ENT,
               ,     , M:la, L:1, K:SPC,    ,           , L:2,   L:1, M:lm,      ,    
        },
        {
          K:TAB,K:1,  K:2, K:3, K:4, K:5,     K:6, K:7, K:8, K:9,  K:0, K:"]",
               ,   ,     ,    ,    ,    ,        ,    ,    ,    ,     ,   K:\`,
               ,   ,     ,    ,    ,    ,        , K:-, K:=,    , K:\\, K:BKS
        },
        {
                , K:F1, K:F2, K:F3, K:F4, K:F5,    K:F6,   K:F7,   K:F8,    K:F9, K:F10,  K:F11,
                ,     ,     ,     ,     ,     ,        ,       ,   K:UP,        ,      ,       ,
                ,     ,     ,     ,     ,     ,        , K:LEFT, K:DOWN, K:RIGHT,      ,        
        },
      },
    },
} `;
const structure = Vason.parse(exampleString);
console.dir(structure, { depth: 1000 });

//
//  ViewController.swift
//  TEST
//
//  Created by bigfish on 2018/8/11.
//  Copyright © 2018年 bigfish. All rights reserved.
//

import UIKit

class ViewController: UIViewController {

    override func viewDidLoad() {
        super.viewDidLoad()
        // Do any additional setup after loading the view, typically from a nib.
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }

    
    
    

}



    
    func googleSignIn(with viewController: UIViewController) {
        if isGoogleConnected {//已经登录
            presenter?.presentUserInfo()
            return
        }
        if GIDSignIn.sharedInstance().hasAuthInKeychain(){//本地缓存信息
            GIDSignIn.sharedInstance().signInSilently()
        } else {
            cViewController = viewController
            presenter?.startActivity()
            let currentScopes: NSArray = GIDSignIn.sharedInstance().scopes as NSArray
            GIDSignIn.sharedInstance().scopes = currentScopes.adding(Auth.Scope)
            GIDSignIn.sharedInstance().signIn()
        }
    }
    
    func signOut() {
        GIDSignIn.sharedInstance().signOut()
        GoogleOAuth2.sharedInstance.clearToken()
        userStorage.user = nil
    }


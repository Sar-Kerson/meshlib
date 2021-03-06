#include <iostream>
#include <math.h>

#include <GL/glut.h>
#include "../MeshLib/core/viewer/Arcball.h"                           /*  Arc Ball  Interface         */
#include "../MeshLib/core/bmp/RgbImage.h"
#include "../MeshLib/core/Geometry/quat.h"
#include "MeshOptimation_v.h"

using namespace MeshLib;
#define DEBUG 0
#define DELAUNAY 0
#define HARMONICMAP 1
#define MESHOPTIMATION 0

/* window width and height */
int win_width, win_height;
int gButton;
int startx, starty;
int shadeFlag = 0;
bool showMesh = true;
bool showUV = true;

/* rotation quaternion and translation vector for the object */
CQrot       ObjRot(0, 0, 1, 0);
CPoint      ObjTrans(0, 0, 0);

/* global mesh */
CMyMesh mesh;
// CHMMesh mesh;

/* arcball object */
CArcball arcball;

int textureFlag = 2;
/* texture id and image */
GLuint texName;
RgbImage image;
bool hasTexture = false;

//copy frame buffer to an image
/*! save frame buffer to an image "snap_k.bmp"
*/
void read_frame_buffer()
{
    static int id = 0;

    GLfloat * buffer = new GLfloat[win_width * win_height * 3];
    assert(buffer);
    glReadBuffer(GL_FRONT_LEFT);
    glReadPixels(0, 0, win_width, win_height, GL_RGB, GL_FLOAT, buffer);

    RgbImage image(win_height, win_width);

    for (int i = 0; i < win_height; i++)
        for (int j = 0; j < win_width; j++)
        {
            float r = buffer[(i*win_width + j) * 3 + 0];
            float g = buffer[(i*win_width + j) * 3 + 1];
            float b = buffer[(i*win_width + j) * 3 + 2];

            image.SetRgbPixelf(i, j, r, g, b);
        }
    delete[]buffer;

    char name[256];
    std::ostringstream os(name);
    os << "snape_" << id++ << ".bmp";
    image.WriteBmpFile(os.str().c_str());

}

/*! initialize bitmap image texture */
void initialize_bmp_texture()
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &texName);
    glBindTexture(GL_TEXTURE_2D, texName);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,   GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    //	int ImageWidth  = image.GetNumRows();
    //	int ImageHeight = image.GetNumCols();
    int ImageWidth = image.GetNumCols();
    int ImageHeight = image.GetNumRows();
    GLubyte * ptr = (GLubyte *)image.ImageData();

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
        ImageWidth,
        ImageHeight,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        ptr);

    if (textureFlag == 1)
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    else if (textureFlag == 2)
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_TEXTURE_2D);
}

/*! setup the object, transform from the world to the object coordinate system */
void setupObject(void)
{
    double rot[16];

    glTranslated(ObjTrans[0], ObjTrans[1], ObjTrans[2]);
    ObjRot.convert(rot);
    glMultMatrixd((GLdouble *)rot);
}

/*! the eye is always fixed at world z = +5 */
void setupEye(void) {
    glLoadIdentity();
    gluLookAt(0, 0, 5, 0, 0, 0, 0, 1, 0);
}

/*! setup light */
void setupLight()
{
    GLfloat lightOnePosition[4] = { 0, 0, 1, 0 };
    GLfloat lightTwoPosition[4] = { 0, 0, -1, 0 };
    glLightfv(GL_LIGHT1, GL_POSITION, lightOnePosition);
    glLightfv(GL_LIGHT2, GL_POSITION, lightTwoPosition);
}

/*! draw axis */
void draw_axis()
{
    glLineWidth(2.0);
    //x axis
    glColor3f(1.0, 0.0, 0.0);	//red
    glBegin(GL_LINES);
    glVertex3d(0, 0, 0);
    glVertex3d(1, 0, 0);
    glEnd();

    //y axis
    glColor3f(0.0, 1.0, 0);		//green
    glBegin(GL_LINES);
    glVertex3d(0, 0, 0);
    glVertex3d(0, 1, 0);
    glEnd();

    //z axis
    glColor3f(0.0, 0.0, 1.0);	//blue
    glBegin(GL_LINES);
    glVertex3d(0, 0, 0);
    glVertex3d(0, 0, 1);
    glEnd();

    glLineWidth(1.0);
}

/*! draw mesh */
void draw_mesh()
{
    if(hasTexture)
        glBindTexture(GL_TEXTURE_2D, texName);


    for (CMyMesh::MeshFaceIterator fiter(&mesh); !fiter.end(); ++fiter)
    {
        glBegin(GL_POLYGON);
        CMyFace * pf = *fiter;
        for (CMyMesh::FaceVertexIterator fviter(pf); !fviter.end(); ++fviter)
        {
            CMyVertex * v = *fviter;
            CPoint & pt = v->point();
            // CPoint & huv = v->huv();
            CPoint2 & uv = v->uv();
            CPoint & rgb = v->rgb();
            CPoint n;
            switch (shadeFlag)
            {
            case 0:
                n = pf->normal();
                break;
            case 1:
                n = v->normal();
                break;
            }
            glNormal3d(n[0], n[1], n[2]);
            glTexCoord2d(uv[0], uv[1]);
            glColor3f(rgb[0], rgb[1], rgb[2]);
            glVertex3d(pt[0], pt[1], pt[2]);
            // glVertex3d(huv[0], huv[1], huv[2]);
        }
        glEnd();
#if HARMONICMAP || MESHOPTIMATION
        glBegin(GL_POLYGON);
        for (CMyMesh::FaceVertexIterator fviter(pf); !fviter.end(); ++fviter)
        {
            CMyVertex * v = *fviter;
            // CPoint & pt = v->point();
            CPoint & huv = v->huv();
            CPoint2 & uv = v->uv();
            CPoint & rgb = v->rgb();
            CPoint n;
            switch (shadeFlag)
            {
            case 0:
                n = pf->normal();
                break;
            case 1:
                n = v->normal();
                break;
            }
            glNormal3d(n[0], n[1], n[2]);
            glTexCoord2d(uv[0], uv[1]);
            glColor3f(rgb[0], rgb[1], rgb[2]);
            // glVertex3d(pt[0], pt[1], pt[2]);
            glVertex3d(huv[0] * 2, huv[1] * 2, huv[2] * 2);
        }
        glEnd();
#endif

    }
}

/*! draw uv */
void draw_uv()
{
    glBindTexture(GL_TEXTURE_2D, texName);

    for (CMyMesh::MeshFaceIterator fiter(&mesh); !fiter.end(); ++fiter)
    {
        glBegin(GL_POLYGON);
        CMyFace * pf = *fiter;
        for (CMyMesh::FaceVertexIterator fviter(pf); !fviter.end(); ++fviter)
        {
            CMyVertex * v = *fviter;
            CPoint2 & uv = v->uv();
            CPoint & rgb = v->rgb();
            CPoint n;
            switch (shadeFlag)
            {
            case 0:
                n = pf->normal();
                break;
            case 1:
                n = v->normal();
                break;
            }
            glNormal3d(n[0], n[1], n[2]);
            glTexCoord2d(uv[0], uv[1]);
            //glColor3f(rgb[0], rgb[1], rgb[2]);
            glVertex3d(uv[0], uv[1], 0);
        }
        glEnd();
    }
}


void draw_sharp_edges()
{
    glLineWidth(2.0);
    glColor3f(1, 0, 0);
    glBegin(GL_LINES);
    for (CMyMesh::MeshEdgeIterator eiter(&mesh); !eiter.end(); ++eiter)
    {
        CMyEdge * pE = *eiter;
        if (pE->sharp() == true)
        {
            CMyVertex * p0 = mesh.edgeVertex1(pE);
            CMyVertex * p1 = mesh.edgeVertex2(pE);
            glColor3f(1.0f, 0.0f, 0.0f);
            glVertex3f(p0->point()[0], p0->point()[1], p0->point()[2]);
            glVertex3f(p1->point()[0], p1->point()[1], p1->point()[2]);
        }
    }
    glEnd();
    glLineWidth(1.0);
}

void drawConvexHull() {
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_LINE_LOOP);
    convexHull::myEdge* edge = convexHull::lboundary[0], *t = edge->he_next();
    glVertex3d((edge->vertex()->point())[0], (edge->vertex()->point())[1], (edge->vertex()->point())[2]);
    while (t->vertex() != edge->source()) {
        glVertex3d((t->vertex()->point())[0], (t->vertex()->point())[1], (t->vertex()->point())[2]);
        t = t->he_next();
    }
    glVertex3d((t->vertex()->point())[0], (t->vertex()->point())[1], (t->vertex()->point())[2]);
    glEnd();
}

void drawMeshPoints() {
    glColor3f(0.0f, 0.0f, 0.0f);
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for(CMyMesh::MeshVertexIterator viter(&mesh); !viter.end(); ++viter) {
        CPoint p = (*viter)->point();
        glVertex3d(p[0], p[1], p[2]);
    }
    glEnd();
}

/*! display call back function
*/
void display()
{
    /* clear frame buffer */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setupLight();
    /* transform from the eye coordinate system to the world system */
    setupEye();
    glPushMatrix();
    /* transform from the world to the ojbect coordinate system */
    setupObject();

    /* draw sharp edges */
    draw_sharp_edges();
    /* draw the mesh */
    if(showMesh)
        draw_mesh();
    if (showUV)
        draw_uv();
    /* draw the axis */
    draw_axis();

    if (convexHull::lboundary.size() > 0) {
        drawConvexHull();
        drawMeshPoints();
    }

    glPopMatrix();
    glutSwapBuffers();
}

/*! Called when a "resize" event is received by the window. */
void reshape(int w, int h)
{
    float ar;
    //std::cout << "w:" << w << "\th:" << h << std::endl;
    win_width = w;
    win_height = h;

    ar = (float)(w) / h;
    glViewport(0, 0, w, h);               /* Set Viewport */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // magic imageing commands
    gluPerspective(40.0, /* field of view in degrees */
        ar, /* aspect ratio */
        1.0, /* Z near */
        100.0 /* Z far */);

    glMatrixMode(GL_MODELVIEW);

    glutPostRedisplay();
}

/*! helper function to remind the user about commands, hot keys */
void help()
{
    printf("w  -  Wireframe Display\n");
    printf("f  -  Flat Shading \n");
    printf("s  -  Smooth Shading\n");
    printf("?  -  Help Information\n");
    printf("esc - quit\n");
}

/*! Keyboard call back function */
void keyBoard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case '1':
        showMesh = !showMesh;
        break;
    case '2':
        showUV = !showUV;
        break;
    case 'f':
        //Flat Shading
        glPolygonMode(GL_FRONT, GL_FILL);
        shadeFlag = 0;
        break;
    case 's':
        //Smooth Shading
        glPolygonMode(GL_FRONT, GL_FILL);
        shadeFlag = 1;
        break;
    case 'w':
        //Wireframe mode
        glPolygonMode(GL_FRONT, GL_LINE);
        break;
    case 't':
        textureFlag = (textureFlag + 1) % 3;
        switch (textureFlag)
        {
        case 0:
            glDisable(GL_TEXTURE_2D);
            break;
        case 1:
            glEnable(GL_TEXTURE_2D);
            glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            break;
        case 2:
            glEnable(GL_TEXTURE_2D);
            glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            break;
        }
        break;
    case 'o':
        read_frame_buffer();
        break;
    case '?':
        help();
        break;
    case 27:
        exit(0);
        break;
    }
    glutPostRedisplay();
}

/*! setup GL states */
void setupGLstate() {
    GLfloat lightOneColor[] = { 0.8, 0.8, 0.8, 1.0 };
    GLfloat globalAmb[] = { .1, .1, .1, 1 };
    GLfloat lightOnePosition[] = { .0, 0.0, 1.0, 1.0 };
    GLfloat lightTwoPosition[] = { .0, 0.0, -1.0, 1.0 };

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.35, 0.53, 0.70, 0);
    glShadeModel(GL_SMOOTH);

    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2);
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);

    glLightfv(GL_LIGHT1, GL_DIFFUSE, lightOneColor);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, lightOneColor);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    glLightfv(GL_LIGHT1, GL_POSITION, lightOnePosition);
    glLightfv(GL_LIGHT2, GL_POSITION, lightTwoPosition);
}

/*! mouse click call back function */
void  mouseClick(int button, int state, int x, int y) {
    /* set up an arcball around the Eye's center
    switch y coordinates to right handed system  */

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        gButton = GLUT_LEFT_BUTTON;
        arcball = CArcball(win_width, win_height, x - win_width / 2, win_height - y - win_height / 2);
    }

    if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN) {
        startx = x;
        starty = y;
        gButton = GLUT_MIDDLE_BUTTON;
    }

    if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        startx = x;
        starty = y;
        gButton = GLUT_RIGHT_BUTTON;
    }
    return;
}

/*! mouse motion call back function */
void mouseMove(int x, int y)
{
    CPoint trans;
    CQrot  rot;

    /* rotation, call arcball */
    if (gButton == GLUT_LEFT_BUTTON)
    {
        rot = arcball.update(x - win_width / 2, win_height - y - win_height / 2);
        ObjRot = rot * ObjRot;
        glutPostRedisplay();
    }

    /*xy translation */
    if (gButton == GLUT_MIDDLE_BUTTON)
    {
        double scale = 10. / win_height;
        trans = CPoint(scale*(x - startx), scale*(starty - y), 0);
        startx = x;
        starty = y;
        ObjTrans = ObjTrans + trans;
        glutPostRedisplay();
    }

    /* zoom in and out */
    if (gButton == GLUT_RIGHT_BUTTON) {
        double scale = 10. / win_height;
        trans = CPoint(0, 0, scale*(starty - y));
        startx = x;
        starty = y;
        ObjTrans = ObjTrans + trans;
        glutPostRedisplay();
    }

}


/*! Normalize mesh
* \param pMesh the input mesh
*/
void normalize_mesh(CMyMesh * pMesh)
{
    CPoint s(0, 0, 0);
    for (CMyMesh::MeshVertexIterator viter(pMesh); !viter.end(); ++viter)
    {
        CMyVertex * v = *viter;
        s = s + v->point();
    }
    s = s / pMesh->numVertices();

    for (CMyMesh::MeshVertexIterator viter(pMesh); !viter.end(); ++viter)
    {
        CMyVertex * v = *viter;
        CPoint p = v->point();
        p = p - s;
        v->point() = p;
    }

    double d = 0;
    for (CMyMesh::MeshVertexIterator viter(pMesh); !viter.end(); ++viter)
    {
        CMyVertex * v = *viter;
        CPoint p = v->point();
        for (int k = 0; k < 3; k++)
        {
            d = (d > fabs(p[k])) ? d : fabs(p[k]);
        }
    }

    for (CMyMesh::MeshVertexIterator viter(pMesh); !viter.end(); ++viter)
    {
        CMyVertex * v = *viter;
        CPoint p = v->point();
        p = p / d;
        v->point() = p;
    }
};

/*! Compute the face normal and vertex normal
* \param pMesh the input mesh
*/
void compute_normal(CMyMesh * pMesh)
{
    for (CMyMesh::MeshVertexIterator viter(pMesh); !viter.end(); ++viter)
    {
        CMyVertex * v = *viter;
        CPoint n(0, 0, 0);
        for (CMyMesh::VertexFaceIterator vfiter(v); !vfiter.end(); ++vfiter)
        {
            CMyFace * pF = *vfiter;

            CPoint p[3];
            CHalfEdge * he = pF->halfedge();
            for (int k = 0; k < 3; k++)
            {
                p[k] = he->target()->point();
                he = he->he_next();
            }

            CPoint fn = (p[1] - p[0]) ^ (p[2] - p[0]);
            pF->normal() = fn / fn.norm();
            n += fn;
        }

        n = n / n.norm();
        v->normal() = n;
    }
};


void init_openGL(int argc, char * argv[])
{
    if(hasTexture)
        image.LoadBmpFile(argv[2]);

    /* glut stuff */
    glutInit(&argc, argv);                /* Initialize GLUT */
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Mesh Viewer");	  /* Create window with given title */
    glViewport(0, 0, 800, 600);

    glutDisplayFunc(display);             /* Set-up callback functions */
    glutReshapeFunc(reshape);
    glutMouseFunc(mouseClick);
    glutMotionFunc(mouseMove);
    glutKeyboardFunc(keyBoard);
    setupGLstate();
    
    if (hasTexture)
        initialize_bmp_texture();

    glutMainLoop();                       /* Start GLUT event-processing loop */
}



/*! main function for viewer
*/
int main(int argc, char * argv[])
{
#if !DEBUG
    if (argc != 2 && argc != 3)
    {
        std::cout << "Usage: input.m [texture.bmp]" << std::endl;
        return -1;
    }

    if (argc > 2)
        hasTexture = true;

    std::string mesh_name(argv[1]);
    if (strutil::endsWith(mesh_name, ".obj"))
    {
        mesh.read_obj(mesh_name.c_str());
    }
    if (strutil::endsWith(mesh_name, ".m"))
    {
        mesh.read_m(mesh_name.c_str());
    }
    if (strutil::endsWith(mesh_name, ".off"))
    {
        mesh.read_off(mesh_name.c_str());
    }

    
    mesh.output_mesh_info();
    mesh.test_iterator();
#if DELAUNAY
    srand(time(NULL));

    for (int i = 0; i < 1000; ++i) {
        double x = ((double)(rand() % 1000000) / 1000000.0),
               y = ((double)(rand() % 1000000) / 1000000.0);
        CPoint p(x, y, 0.0);
        delaunayTriangulation::insertVertex(mesh, p);
    }

    mesh.write_m("delaunay.m");

    convexHull::makeConvexHull(mesh);

    normalize_mesh(&mesh);
    compute_normal(&mesh);
    //----------------------TEST GAUSS-BONNET--------------------------
    std::cout << "The result of the test of Gauss-Bonnet-Theorom is: " << gauss_bonnet::checkG_B(&mesh) << std::endl;
    //-----------------------------------------------------------------
    mesh.output_mesh_info();
    mesh.test_iterator();
#endif

#if HARMONICMAP
    generateHarmornicMap(mesh);
    // for( CMyMesh::MeshVertexIterator viter( &mesh ); !viter.end(); ++ viter )
    // {
    //     CMyVertex * pV = *viter;
    //     pV->point() = pV->huv();
    // }
#endif

#if MESHOPTIMATION
    generateHarmornicMap(mesh);
    meshOptimation::legalizeMesh(mesh);
#endif
    mesh.output_mesh_info();
    mesh.test_iterator();

    init_openGL(argc, argv);
#else 
    Eigen::Matrix3f A, B;
    Eigen::MatrixXf C(5, 3);
    Eigen::Vector3f b;
    A << 1, 2, 3,
         4, 5, 6,
         7, 8, 10;
    B << 5, 5, 5,
         6, 6, 6,
         7, 7, 7;
    C << 8, 8, 8,
         9, 9, 9,
         10, 10, 10,
         11, 11, 11,
         12, 12, 12;
    b << 3, 3, 4;
    // std::cout << "for A x = b: " << "\n"
    //      << "A: \n" << A << "\n"
    //      << "b: \n" << b << "\n";
    // Eigen::Vector3f x = A.colPivHouseholderQr().solve(b);
    // std::cout << "solution x: \n" << x << "\n";
    // A.row(0).swap(A.row(1));
    // std::cout << "A': \n" << A << "\n";
    // std::cout << "B': \n" << B << "\n";
    Eigen::Vector3d v1(3); v1 << 0.5, 0.5, -0.9;
    Eigen::Vector3d v2(3); v2 << 1, 0, -0.75;

    double dot = v1.dot(v2);
    std::cout << "dot: " << dot << "\n";

    Eigen::MatrixXd v = v1 * v2.transpose();
    std::cout << v << "\n";
    std::cout << "det: " << v.determinant() << "\n";

    std::cout << mod(v1.cross(v2)) << "\n";

    // std::map<CVertex*, Eigen::Vector3f> g;
    // CVertex * v1 = new CVertex(),
    //         * v2 = new CVertex(),
    //         * v3 = new CVertex();
    // v1->point() = CPoint(0,0,0);
    // v2->point() = CPoint(0,0,1);
    // v3->point() = CPoint(0,0,2);
    // Eigen::Vector3f f1; f1 << 1,1,0;
    // Eigen::Vector3f f2; f2 << 1,1,1;
    // Eigen::Vector3f f3; f3 << 1,1,2;
    // g.insert(std::make_pair(v1,f1));
    // g.insert(std::make_pair(v2,f2));
    // g.insert(std::make_pair(v3,f3));
    // for (std::map<CVertex*, Eigen::Vector3f>::iterator it = g.begin(); it != g.end(); ++ it) {
    //     std::cout << it->first->point() << "&" << it->second << "\n";
    // }
    std::cout << "--------test----------------\n";
    std::vector<Eigen::Triplet<double> > tris;
    for (int i = 3; i > 0; --i) {
        for (int j = 3; j > 0; --j) {
            tris.push_back(Eigen::Triplet<double>(i-1, j-1, i+j));
        }
    }
    tris.push_back(Eigen::Triplet<double>(2, 2, 100));
    tris.push_back(Eigen::Triplet<double>(1, 1, 1000));
    tris.push_back(Eigen::Triplet<double>(1, 1, 10000));
    Eigen::SparseMatrix<double> Ls(3, 3);
    Ls.setFromTriplets(tris.begin(), tris.end());
    std::cout << "ls:\n" << Ls << "\n";
    std::cout << "--------test----------------\n";
    std::list<int> vecs;
    for (int i = 0; i < 5; ++i) {
        vecs.push_back(i);
    }
    for (int i = 0; i <vecs.size(); ++i) {
        std::list<int>::iterator it = vecs.begin();
        std::advance(it, i);
        std::cout << *it << "\n";
        vecs.push_back(*it + 1);
        if (vecs.size() > 20) break;
    }

#endif
    return 0;
}
